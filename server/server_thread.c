//#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */
#include "../array/dyn_array.c"
#include "server_thread.h"
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include<pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>

enum { NUL = '\0' };

enum {
  /* Configuration constantes.  */
  max_wait_time = 5,
  server_backlog_size = 5
};

pthread_mutex_t lockAvailable = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockMax = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockAllocated = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockCountAccep = PTHREAD_MUTEX_INITIALIZER;//nombre de requete acceptées
pthread_mutex_t lockCouWait = PTHREAD_MUTEX_INITIALIZER;//nombre de requete acceptées avec mise en attente
pthread_mutex_t lockCouInvalid = PTHREAD_MUTEX_INITIALIZER;//nombre de requete erronees
pthread_mutex_t lockCouDispa = PTHREAD_MUTEX_INITIALIZER;//nombre de clients qui se sont terminés correctement
pthread_mutex_t lockReqPro= PTHREAD_MUTEX_INITIALIZER;//nombre total de requête traites
pthread_mutex_t lockClientEnd = PTHREAD_MUTEX_INITIALIZER;//nombre de clients ayant envoye le message CLO
pthread_mutex_t lockClientWait = PTHREAD_MUTEX_INITIALIZER;//Clients a qui j'ai dit de wait 
pthread_mutex_t lockBanker = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockClientFini = PTHREAD_MUTEX_INITIALIZER;


int server_socket_fd;

// Nombre de client enregistrés.
int nbClients=0;

int nbRessources;

struct sockaddr_in thread_addr;
// Variable du journal.
// Nombre de requêtes acceptées immédiatement (ACK envoyé en réponse à REQ).
unsigned int count_accepted = 0;
// Nombre de requêtes acceptées après un délai (ACK après REQ, mais retardé).
unsigned int count_wait = 0;
// Nombre de requête erronées (ERR envoyé en réponse à REQ).
unsigned int count_invalid = 0;
// Nombre de clients qui se sont terminés correctement
// (ACK envoyé en réponse à CLO).
unsigned int count_dispatched = 0;
// Nombre total de requête (REQ) traités.
unsigned int request_processed = 0;
// Nombre de clients ayant envoyé le message CLO.
unsigned int clients_ended = 0;

//------------ Ajouter vos structures de données partagées, ici.---------

int *provided; // Valeurs reçues du PRO
int *available; // Valeurs courantes
int **allocated; // Tableau des ressources données par client
int **max; // Tableau des ressources initiées par client

// Array de la longueur du nombre de client
// False si pas en attente, true sinon
bool *clientWaiting;

//Array de la longueur du nombre de client
// False si pas fini, true sinon
bool *clientFini;

//--Définition méthodes (pas dans header à cause dépendance--
struct array_t_string *parseInput(char *input);
void st_banker(int tidClient, struct array_t_string *input, FILE* socket_w);
int stateSafe(struct array_t_string *input,int tidClient);

/*
    Permet incrémentation rapide d'une variable globale
*/  
void lockIncrementUnlock(pthread_mutex_t mut, unsigned int *count){
  pthread_mutex_lock(&mut);
  *count += 1;
  pthread_mutex_unlock(&mut); 
}

/*
    Prend le message envoyé et le concatène à ERR puis l'envoie sur le socket
    Incrémente le compteur approprié
*/
void sendErreur(const char *message, FILE *socket_w){
  printf("Serveur va envoyer ERR %s \n",message);
  fprintf (socket_w, "ERR %s \n", message);
  fflush(socket_w);
  lockIncrementUnlock(lockCouInvalid,&count_invalid);
  return;
}

/*
    Prend le temps envoyé et le concatène à WAIT puis l'envoie sur le socket
    Dans le tableau de clients qui attendent, met true à ce client
*/
void sendWait(int temps,FILE *socket_w,int tid_client){
  printf("Serveur va envoyer WAIT %d \n",temps);
  fprintf (socket_w, "WAIT %d \n",temps);
  fflush(socket_w);
  pthread_mutex_lock(&lockClientWait);
  clientWaiting[tid_client] = true;
  pthread_mutex_unlock(&lockClientWait); 
  return;
}

/*
    Envoie ACK sur le socket
    Si le client attendait, on l'enlève du tableau
    Incrémente les compteurs nécessaires
*/
void sendAck(FILE *socket_w, int clientTid, int req){
  printf("Serveur va envoyer ACK à client %d \n", clientTid);
  fprintf (socket_w, "ACK \n");
  fflush(socket_w);
  pthread_mutex_lock(&lockClientWait);
  if ( clientTid != -1 && clientWaiting[clientTid])
  {
  	clientWaiting[clientTid]=false;
  	pthread_mutex_unlock(&lockClientWait);
  	lockIncrementUnlock(lockCouWait,&count_wait);
  }

  else{
    if(req){
        lockIncrementUnlock(lockCountAccep,&count_accepted);
    }
  	pthread_mutex_unlock(&lockClientWait);
  	
  }

  return;
}

/*
    Transforme le char* en un array_t_string
    Permet d'accéder aux mots et aux valeurs rapidement
*/
struct array_t_string *parseInput(char *input){
  char *reste;
  char *token = strtok_r(input,"\n",&reste);
  struct array_t_string *array = new_arrayString(15);
  char *reste1; 
  token = strtok_r(token," ",&reste1);
  if(push_backString(array,token)==-1){
  		perror("parseInput");
  	}
  int i =0;
  //inspiré par https://stackoverflow.com/questions/2227198/segmentation-fault-when-using-strtok-r?newreg=89b070f8caf842f69e47b0b4774f7748
  while(token != NULL){
  	token = reste1;
  	token = strtok_r(token," ",&reste1);
  	 i +=1;
  	 if(token != NULL && push_backString(array,token)==-1){
  		perror("parseInput");
  	}
  }
  return array;
}

//Met valeurs de base
void fillMatrix(){
    for (int i=0;i<nbClients;i++){
        for (int j=0; j<nbRessources;j++){
            allocated[i][j] = 0;
            max[i][j]=0;
        }
    }
}

/*
    Fait la première partie du banquier, soit de vérifier la demande
    Algo inspiré des notes de cours IFT2245
    Note : le serveur ne valide pas le format des requêtes 
    (i.e si on met des lettres), puisque ces valeurs sont générées
    par nous-mêmes. Cela allourdissait également le code
*/
void st_banker(int tidClient, struct array_t_string *input, FILE* socket_w){
    
    pthread_mutex_lock(&lockBanker);
    pthread_mutex_lock(&lockAllocated);
    pthread_mutex_lock(&lockMax);
    pthread_mutex_lock(&lockAvailable);

      for (int i=0; i<nbRessources;i++){
            // Demande de ressources
            if (atoi(input->data[i+2]) > 0){
                printf("BANQUIER : DEMANDE %d | DEJA ALLOUE : %d | MAX CLIENT : %d \n",
                     atoi(input->data[i+2]), allocated[tidClient][i], max[tidClient][i]);
                

                //Allocation + ask doit être <= max
                if (
                    (allocated[tidClient][i] + 
                              atoi(input->data[i+2])) >
                              max[tidClient][i]
                    ){
                    pthread_mutex_unlock(&lockAllocated);
                    pthread_mutex_unlock(&lockMax);
                    pthread_mutex_unlock(&lockAvailable);
                    sendErreur("Une valeur demandée trop haute", socket_w);
                    pthread_mutex_unlock(&lockBanker);
                    printf("BANKER - REQ DENIED FOR CLIENT %d \n",tidClient);
                    return;
                }

                //Serveur n'a pas assez de ressources
                if(atoi(input->data[i+2]) > available[i]){
                    pthread_mutex_unlock(&lockBanker);
                    printf("BANKER - REQ MUST WAIT FOR CLIENT %d \n", tidClient);
                    pthread_mutex_unlock(&lockAllocated);
                    pthread_mutex_unlock(&lockMax);
                    pthread_mutex_unlock(&lockAvailable);
                    sendWait(max_wait_time,socket_w,tidClient);
                    return;
                }


            // Libération de ressources
            }else if (atoi(input->data[i+2]) < 0){
                if (atoi(input->data[i+2]) > allocated[tidClient][i]){
                    pthread_mutex_unlock(&lockAllocated);
                    pthread_mutex_unlock(&lockMax);
                    pthread_mutex_unlock(&lockAvailable);
                    sendErreur("Une valeur libérée trop haute", socket_w);
                    pthread_mutex_unlock(&lockBanker);
                    printf("BANKER - REQ DENIED FOR CLIENT %d \n",tidClient);  
                    return;
                }
            }
            
      }

     //System prétend qu'il a alloué les ressources
    for (int i=0; i<nbRessources;i++){
        available[i] -= atoi(input->data[i+2]);
        allocated[tidClient][i] += atoi(input->data[i+2]);
    }

    //Vérifie l'état sécure
    int safe = stateSafe(input,tidClient);
    if (!safe){
        sendWait(max_wait_time,socket_w,tidClient);
    }else{
        sendAck(socket_w,tidClient,1);
    }
    pthread_mutex_unlock(&lockAllocated);
    pthread_mutex_unlock(&lockMax);
    pthread_mutex_unlock(&lockAvailable);
    pthread_mutex_unlock(&lockBanker); 
}

/*
    Permet la gestion du BEG et du PRO initial
    Mis dans une fonction pour la réutilisabilité du code
    command = 1 == BEG
    command = 2 == PRO
*/
void openAndGetline(int command, socklen_t socket_len){
  int socketFd;
  int tag;
  printf("Serveur accepting... \n");
  FILE *socket_r;
  FILE *socket_w;
  struct array_t_string *input;
  do{
    tag = 0;
    socketFd = -1;
    while( socketFd == -1) { 
      socketFd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, 
                        &socket_len);
    }

    socket_r = fdopen (socketFd, "r");
    socket_w = fdopen (socketFd, "w");

    char* args='\0';
    size_t args_len=0;
    
    //TODO: Voir si des cas où args != NULL mais return -1 comme dans client
    if(getline(&args,&args_len,socket_r) == -1){
      sendErreur("Pas reçu de commande",socket_w);
      if (args) {free(args);}
      fclose (socket_r);
      fclose (socket_w);
      continue;
    }else{

      input = parseInput(args);
      //BEG
      if(command==1){
          if(array_get_size(input) != 3 ){
            sendErreur("Trop ou pas assez d'arguments (BEG nbRess nbCli)",socket_w);
            fclose (socket_r);
            fclose (socket_w);
            if (args) {free(args);}
            continue;
          }
          //Si on a bien reçu un BEG
          if (strcmp(input->data[0],"BEG\n") !=0){
              printf("BEG reçu \n");
              tag = 1;  // Va quitter la loop
              
               // Assigne les valeurs, le client est responsable
               // De faire la vérification 
              nbRessources = atoi(input->data[1]);
              nbClients = atoi(input->data[2]);  

              available = malloc (nbRessources * sizeof(int));
              provided = malloc (nbRessources * sizeof(int));
              clientWaiting = calloc(nbClients, sizeof(bool));
              clientFini = calloc(nbClients, sizeof(bool));

              for (int i = 0; i < nbClients; ++i){
              	clientWaiting[i] = false;
                clientFini[i] = false;
              }
          }

      //PRO
      }else if (command==2){
          if(array_get_size(input) != nbRessources + 1){
            sendErreur("Trop ou pas assez d'arguments(PRO res1 res2 ...)",
                        socket_w);
            fclose (socket_r);
            fclose (socket_w);
            if (args) {free(args);}
            continue;
          }
          //Si on a bien reçu un PRO
          if (strcmp(input->data[0],"PRO") == 0){
                printf("PRO reçu \n");
                tag = 1; //Quitte la loop

                //Assigne les valeurs
                for (int i=0;i<nbRessources;i++){
                    provided[i] = atoi(input->data[i+1]);
                    available[i] = provided[i];
                }
                
                //Alloue l'espace des 'matrices'
                allocated = malloc (nbClients * sizeof (int*));
                max = malloc (nbClients * sizeof (int*));

                for (int i=0; i<nbClients; i++){
                    allocated[i] = malloc ((nbRessources+1) * sizeof (int));
                    max[i] = malloc ((nbRessources+1) * sizeof (int));
                }

                fillMatrix();
          }
      }
    }

   //Si on n'a pas recu commande voulue
   if(!tag){
        fclose (socket_w);
        fclose (socket_r);
   }
   if (args) {free(args);}
  }while (!tag); //On reloop tq pas un beg/pro

  sendAck(socket_w,-1,0);

  //Libère la mémoire
  if (input) {delete_array_string(input);}
  fclose (socket_w);
  fclose (socket_r);
}


static void sigint_handler(int signum){
  // Code terminaison.
  printf("je recois un signal d'interruption\n");
  accepting_connections = 0;//je n'acccepte plus de connection
}

void st_init (){
  // Handle interrupt
  signal(SIGINT, &sigint_handler); 

  // Initialise le nombre de clients connecté.
  nbClients = 0;

  socklen_t socket_len = sizeof (thread_addr);

  openAndGetline(1, socket_len); 
  openAndGetline(2, socket_len); 
  
}

int st_wait() {
  struct sockaddr_in thread_addr;

  socklen_t socket_len = sizeof (thread_addr);

  int thread_socket_fd = -1;

  int end_time = time (NULL) + max_wait_time; 

  while(thread_socket_fd < 0 && accepting_connections) {
    thread_socket_fd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, 
                              &socket_len);
    
    //Si j'ai dépassé ou egal le temps que de fin d'attente
    if (time(NULL) >= end_time) {
      break;
    }
  }
  return thread_socket_fd; 
}

//Fonctions utiles
void unlockAndDestroy(pthread_mutex_t mut){
    pthread_mutex_unlock(&mut);
    pthread_mutex_destroy(&mut);
}

//Fonctions utiles
void lockUnlockDestroy(pthread_mutex_t mut){
    pthread_mutex_lock(&mut);
    unlockAndDestroy(mut);
}

//Permet de libérer une 'matrice'
void myFree(int **array){
  for (int i = 0; i < nbClients; ++i)
  {
    free(array[i]);
  }
 
  free(array);
}

/*
    Cette fonction s'occupe de gérer la réception d'un end
    Elle libère les ressources et détruit les mutex
*/
bool commEND (FILE *socket_r,FILE *socket_w){
      pthread_mutex_lock(&lockCouDispa);
      printf("NbClients = %d et  count_dispatched = %d\n",nbClients,
            count_dispatched );

      //On doit avoir reçu tout le monde
      if (nbClients==count_dispatched){
        pthread_mutex_lock(&lockAvailable);
       
        for (int i = 0; i < nbRessources; ++i){
          //Cas où dernier req n'a pas bien fonctionné
          if(provided[i] != available[i]){
            sendErreur("Des ressources n'ont pas ete liberées",socket_w);
            return false ;
          }
        }
        sendAck(socket_w,-1,0);
        
        free(available);
        free(provided);
        
        //Detruit les mutex et libère la mémoire pour mettre fin au serveur
        sigint_handler(1);
        unlockAndDestroy(lockAvailable);  
        pthread_mutex_lock(&lockClientFini);
        free(clientFini);
        unlockAndDestroy(lockClientFini);
        pthread_mutex_lock(&lockClientWait);
        free(clientWaiting);
        unlockAndDestroy(lockClientWait); 
        pthread_mutex_lock(&lockMax);
        myFree(max);
        unlockAndDestroy(lockMax);
        pthread_mutex_lock(&lockAllocated);
        myFree(allocated);
        unlockAndDestroy(lockAllocated);
        lockUnlockDestroy(lockCountAccep); 
        lockUnlockDestroy(lockCouWait);  
        lockUnlockDestroy(lockCouInvalid);    
        unlockAndDestroy(lockCouDispa);    
        lockUnlockDestroy(lockReqPro);         
        lockUnlockDestroy(lockClientEnd);       
        return true; 
      
    }
    sendErreur("Il reste des clients \n",socket_w);
    return false;
}

/*
    Cette fonction fait la gestion des INI, REQ et CLO
    Elle vérifie si les requêtes ont un format valide
    Elle appelle le banquier lorsque nécessaire
    Elle incrémente les compteurs 
*/
void st_process_requests (server_thread * st, int socket_fd){
  FILE *socket_r = fdopen (socket_fd, "r");
  FILE *socket_w = fdopen (socket_fd, "w");
  
  //Gestion user input
  struct array_t_string *input;
  char* args;
  size_t args_len;

  bool liberer;

  while (true){
    liberer = false;  
    args_len=0;

    if (socket_r != NULL)
	  {
         args='\0';
	  	 if(getline(&args,&args_len,socket_r) == -1){
	      liberer = true; 
	      if (args) {free(args);}
	      break;
	    }
	  }
	  else{
        //Utilisé pour empêcher un segfault mystérieux en lien avec le socket
	  	liberer =true;
	  	break;
	  }
	   
	printf("Serveur a recu : %s  sur  %d \n",args,socket_fd);
    input= parseInput(args);

    //Si on a reçu un end, on appelle la fonction de clôture
    if(strcmp(input->data[0],"END") == 0){
      commEND(socket_r,socket_w);
      break;
    }

    //Les autres commandes ont au moins 1 argument
    if(array_get_size(input)  < 2 ){
        sendErreur("Pas assez d'arguments",socket_w);
        if (input) {
        	delete_array_string(input);
        }
        
        if (args) {free(args);}
        fclose (socket_r);
        fclose (socket_w);
        return;
    }

    int tidClient = atoi(input->data[1]);
    printf(" du client %d \n",tidClient);

    if( strcmp(input->data[0],"INI") == 0){
        pthread_mutex_lock(&lockMax);
       for (int i=0; i<nbRessources;i++){
           //Si INI demande plus que ce qu'on a
           if (atoi(input->data[i+2]) > provided[i]){
                sendErreur("Demande trop grande",socket_w);
                if (input) {
                	delete_array_string(input);
                }
                 
                if (args) {free(args);}
                fclose (socket_r);
                fclose (socket_w);
                return;
           }else{
                //Assigne la valeur max du client à ce qu'il demande
                max[tidClient][i] = atoi(input->data[i+2]); 
            }
        }

       pthread_mutex_unlock(&lockMax);
       sendAck(socket_w,tidClient,0);
       break;

    }else if (strcmp(input->data[0],"REQ") == 0){
      lockIncrementUnlock(lockReqPro,&request_processed);
      st_banker(tidClient,input, socket_w); //Appel le banquier
      break;
    
    }else if(strcmp(input->data[0],"CLO") == 0){
          for(int i=0;i<nbRessources;i++){
                //Avant de CLO on doit avoir tout libéré
                if (allocated[tidClient][i]!=0){
                    printf("Valeur pas à 0 %d pour client %d \n",i,tidClient);
                    sendErreur("Doit avoir libéré tout",socket_w);
                    if (input) {
                    	delete_array_string(input);
                   	}

                    if (args) {free(args);}
                    fclose (socket_r);
                    fclose (socket_w);
                    return;
                }  
          }
        lockIncrementUnlock(lockClientEnd,&clients_ended);
        lockIncrementUnlock(lockCouDispa,&count_dispatched);
        sendAck(socket_w,tidClient,0);
        clientFini[tidClient] = true;
        break;
    }else{
      sendErreur("ERR commande inconnu",socket_w); 
      break;  
    }        

  }
if (input && !liberer) {delete_array_string(input);}
if (args  && !liberer ) {free(args);}
if (!liberer){fclose (socket_r);}
if (!liberer){fclose (socket_w);} //Fantome du segfault
}



void *st_code (void *param){
  server_thread *st = (server_thread *) param;

  int thread_socket_fd;

  // Boucle de traitement des requêtes.
  while (accepting_connections)
  {
    thread_socket_fd = -1;
    // Wait for a I/O socket.
    thread_socket_fd = st_wait();

    if (thread_socket_fd < 0)
    {
      fprintf (stderr, "Time out on thread %d.\n", st->id);
      continue;
    }

    if (thread_socket_fd > 0){
        st_process_requests (st, thread_socket_fd);
        close (thread_socket_fd);
    }
  }

  return st;
}

//Algo inspiré de geeksforgeeks.org/operating-system-bankers-algorithm
int stateSafe(struct array_t_string *input,int tidClient){
    int work[nbRessources];
    int finish[nbClients];

    //Si des clients ont fini (CLO) on les met directement à true
    pthread_mutex_lock(&lockClientFini);
    for (int i = 0 ; i < nbClients; i++){
        finish[i] = clientFini[i];
    }

    pthread_mutex_unlock(&lockClientFini);
    for (int i = 0; i < nbRessources; i++){
        work[i] = available[i];
    }

    int count = 0;
    while(count < nbClients){
        bool found = false;
        for(int i = 0 ; i < nbClients ; i ++){
            //Éviter de repasser deux fois dans le même
            if (!finish[i]){
                int j;
                for (j = 0; j < nbRessources; j++){
                    // Si besoin > dispo + allocation précédente
                    // Alors fonctionnera pas, on break
                    if ((max[i][j]-allocated[i][j]) > work[j]){
                        break;
                    }
                }

                //Si on a jamais break de la loop précédente
                if (j == nbRessources){
                    for(int k = 0;k < nbRessources; k++){
                        //Assigne les ressources
                        work[k] += allocated[i][k];
                        count++;
                    }
                    //On a trouvé un i qui satisfait la condition
                    finish[i] = 1;
                    found=true;
                   
                }
            }
        }
        //Aucun client i qui peut fini, état non sûr
        if (!found){
            for (int k=0; k<nbRessources;k++){
                //On efface ce qu'on avait fait avant de rentrer
                available[k] += atoi(input->data[k+2]);
                allocated[tidClient][k] -= atoi(input->data[k+2]);
            }
            return 0;
        }
    }
    return 1;
}


//
// Ouvre un socket pour le serveur.
//
void st_open_socket (int port_number){
  #ifndef SOCK_NONBLOCK
 server_socket_fd = socket (AF_INET, SOCK_STREAM, 0);
  #else
 server_socket_fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

  #endif

 if (server_socket_fd < 0) {
   perror ("ERROR opening socket");
   exit(1);
 }
  #ifndef SOCK_NONBLOCK // If SOCK_NONBLOCK not available
  {
   int sockopt = fcntl(server_socket_fd, F_GETFL);
   fcntl(server_socket_fd, F_SETFL, sockopt | O_NONBLOCK);
  }
  #endif

 if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {    perror("setsockopt()");
 perror("setsockopt()");
 exit(1);
 }

 struct sockaddr_in serv_addr;
 memset (&serv_addr, 0, sizeof (serv_addr));
 serv_addr.sin_family = AF_INET;
 serv_addr.sin_addr.s_addr = INADDR_ANY;
 serv_addr.sin_port = htons (port_number);

 if (bind(server_socket_fd, (struct sockaddr *) &serv_addr,sizeof (serv_addr)) < 0)
 perror ("ERROR on binding");

 listen (server_socket_fd, server_backlog_size);
}

//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void st_print_results (FILE * fd, bool verbose){
  if (fd == NULL) fd = stdout;
  if (verbose)
  {
    fprintf (fd, "\n---- Résultat du serveur ----\n");
    fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
    fprintf (fd, "Requêtes en attentes : %d\n", count_wait);
    fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
    fprintf (fd, "Clients : %d\n", count_dispatched);
    fprintf (fd, "Requêtes traitées: %d\n", request_processed);
  }
  else
  {
    fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_wait,
    count_invalid, count_dispatched, request_processed);
  }
}

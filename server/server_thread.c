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
  /* Configuration constants.  */
  max_wait_time = 30,
  server_backlog_size = 5
};

void st_banker(int tidClient, struct array_t_string *input, FILE* socket_w);
void cleanBanker(struct array_t_string *input, FILE* socket_w);
pthread_mutex_t lockNbClient = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockResLibres = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockMax = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockAllouer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockCountAccep = PTHREAD_MUTEX_INITIALIZER;//nombre de requete accepter
pthread_mutex_t lockCouWait = PTHREAD_MUTEX_INITIALIZER;//nombre de requete accepter avec mise en attente
pthread_mutex_t lockCouInvalid = PTHREAD_MUTEX_INITIALIZER;//nombre de requete erronees
pthread_mutex_t lockCouDispa = PTHREAD_MUTEX_INITIALIZER;//nombre de clients qui se sont terminés correctement
pthread_mutex_t lockReqPro= PTHREAD_MUTEX_INITIALIZER;//nombre total de requête traites
pthread_mutex_t lockClientEnd = PTHREAD_MUTEX_INITIALIZER;//nombre de clients ayant envoye le message CLO
pthread_mutex_t lockClientWait = PTHREAD_MUTEX_INITIALIZER;//Clients a qui j'ai dit de wait 
pthread_mutex_t lockBesoin = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t locknbChaqRess = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockBanker = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockStrTock = PTHREAD_MUTEX_INITIALIZER;


int server_socket_fd;

// Nombre de client enregistré.
int nbClients=0;

int *nbChaqueRess;
int nbRessources;

void  processBEG( socklen_t socket_len );
void processPRO( socklen_t socket_len);
int stateSafe(struct array_t_string *input,int tidClient);
//struct array_t clientQuiWait; //Clients a qui j'ai dit de wait 
//struct array_t max;
//struct array_t allouer;
//struct array_t besoin;

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
// TODO: Ajouter vos structures de données partagées, ici.
int *available;
int **allocated;
int **max;
int **need;
bool *clientWaiting;
void lockIncrementUnlock(pthread_mutex_t mut, int count){
  pthread_mutex_lock(&mut);
  count += 1;
  pthread_mutex_unlock(&mut); 
}


void sendErreur(const char *message, FILE *socket_w){
  printf("Serveur va envoyer ERR %s \n",message);
  fprintf (socket_w, "ERR %s \n", message);
  fflush(socket_w);

  lockIncrementUnlock(lockCouInvalid,count_invalid);
  lockIncrementUnlock(lockReqPro,request_processed);

  return;
}

void sendWait(int temps,FILE *socket_w,int tid_client){
  printf("Serveur va envoyer WAIT %d \n",temps);
  fprintf (socket_w, "WAIT %d \n",temps);
  fflush(socket_w);
  pthread_mutex_lock(&lockClientWait);
  clientWaiting[tid_client] = true;
  pthread_mutex_unlock(&lockClientWait);
  //lockIncrementUnlock(lockClientWait,count_invalid);
  lockIncrementUnlock(lockReqPro,request_processed);
  //pthread_mutex_lock(&lockClientWait);
  //push_back(&clientWaiting,tid_client);
  //pthread_mutex_unlock(&lockClientWait);
 
 return;
}

void sendAck(FILE *socket_w, int clientTid){
  printf("Serveur va envoyer ACK à client %d \n", clientTid);
  fprintf (socket_w, "ACK \n");
  fflush(socket_w);
  pthread_mutex_lock(&lockClientWait);
  if (clientWaiting[clientTid])
  {
  	clientWaiting[clientTid]=false;
  	pthread_mutex_unlock(&lockClientWait);
  	lockIncrementUnlock(lockCouWait,count_wait);
  }

  else{
  	pthread_mutex_unlock(&lockClientWait);
  	lockIncrementUnlock(lockCountAccep,count_accepted);
  }

  lockIncrementUnlock(lockReqPro,request_processed);  
  
  //TODO: Redo this -> fait a la ligne 118 ?
/*
  pthread_mutex_lock(&lockClientWait);

  if ( clientTid != -1 &&estDansListe(&clientQuiWait,clientTid)){
    lockIncrementUnlock(lockCouWait,count_wait);  
    deleteClientInArray(&clientQuiWait,clientTid);
  }
  pthread_mutex_unlock(&lockClientWait);

  lockIncrementUnlock(lockReqPro,request_processed);    
*/
  return;
}

struct array_t_string *parseInput(char *input){
  //char* copy; 
  //strncpy(copy, input, sizeof(input));
  //copy[sizeof(input) - 1] = '\0';
 
  char *reste;
  char *token = strtok_r(input,"\n",&reste);
  struct array_t_string *array = new_arrayString(5);
  char *reste1; 
  token = strtok_r(token," ",&reste1);
  if(push_backString(array,token)==-1){
  		perror("parseInput");
  	}
  int i =0;
  //inspirer par https://stackoverflow.com/questions/2227198/segmentation-fault-when-using-strtok-r?newreg=89b070f8caf842f69e47b0b4774f7748
  while(token != NULL){
  	token = reste1;
  	token = strtok_r(token," ",&reste1);
  	 i +=1;
  	 if(token != NULL && push_backString(array,token)==-1){
  		perror("parseInput");
  	}
  }
  return array;


 /* pthread_mutex_lock(&lockStrTock);
  char *token = strtok(input,"\n");
  pthread_mutex_unlock(&lockStrTock);

  struct array_t_string *array = new_arrayString(5);

  pthread_mutex_lock(&lockStrTock);
  token = strtok(token," ");

  int i =0;

  while(token != NULL){
  	if(push_backString(array,token)==-1){
  		perror("parseInput");
  	}
  	token = strtok(NULL," ");
  	 i +=1;
  }

  pthread_mutex_unlock(&lockStrTock);
  return array;*/
}

void closeStream(FILE *sockr, FILE *sockw){
    fclose(sockr);
    fclose(sockw);
}

void freeValues(char *args, struct array_t_string *input){
    if (args) {free(args);}
    if (input) {delete_array_string(input);}
}

void fillMatrix(){
    for (int i=0;i<nbClients;i++){
        for (int j=0; j<nbRessources;j++){
            allocated[i][j] = 0;
            max[i][j]=0;
            need[i][j]=0;
        }
    }
}



void st_banker(int tidClient, struct array_t_string *input, FILE* socket_w){
    //printf("Le banquier commence \n");
    pthread_mutex_lock(&lockBanker);
      //Valid format
      for (int i=0; i<nbRessources;i++){
            if (atoi(input->data[i+2]) > 0){
                printf("BANQUIER : DEMANDE %d | DEJA ALLOUE : %d | MAX CLIENT : %d \n",atoi(input->data[i+2]), allocated[tidClient][i], max[tidClient][i]);
                //Ask + allocated doit être <= max
                if (
                    (allocated[tidClient][i] + 
                              atoi(input->data[i+2])) >
                              max[tidClient][i]
                    ){
                    sendErreur("Une valeur demandée trop haute", socket_w);
                    pthread_mutex_unlock(&lockBanker);
                    printf("BANKER - REQ DENIED FOR CLIENT %d \n",tidClient);
                    return;
                }

                //Serveur n'a pas assez de ressources
                if(atoi(input->data[i+2]) > available[i]){
                    pthread_mutex_unlock(&lockBanker);
                    printf("BANKER - REQ MUST WAIT FOR CLIENT %d \n", tidClient);
                    count_wait++;
                    sendWait(max_wait_time,socket_w,tidClient);
                    return;
                }


            //Req nombre négatif
            }else if (atoi(input->data[i+2]) < 0){
                if (atoi(input->data[i+2]) > allocated[tidClient][i]){
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
        need[tidClient][i] -= atoi(input->data[i+2]);
    }


    int safe = stateSafe(input,tidClient);
    if (!safe){
        count_wait++;
        sendWait(max_wait_time,socket_w,tidClient);
    }else{
        count_accepted++;
        sendAck(socket_w,tidClient);
    }
    pthread_mutex_unlock(&lockBanker); 
}


void openAndGetline(int command, socklen_t socket_len){
  int socketFd = -1;
  int tag;
  printf("Serveur accepting... \n");
  FILE *socket_r;
  FILE *socket_w;
  struct array_t_string *input;
  do{
    tag = 0;

    while( socketFd == -1) { 
      socketFd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
    }

    socket_r = fdopen (socketFd, "r");
    socket_w = fdopen (socketFd, "w");

    char *args; 
    size_t args_len=0;

    printf("About to getline \n");
    if(getline(&args,&args_len,socket_r) == -1){
      sendErreur("Pas reçu de commande",socket_w);
      if (args) {free(args);}
      closeStream(socket_r,socket_w);
      continue;
    }else{
      input = parseInput(args);
      //BEG
      if(command==1){
          if(array_get_size(input) != 3 ){
            sendErreur("Trop ou pas assez d'arguments (BEG nbRess nbCli)",socket_w);
            freeValues(args, input);
            closeStream(socket_r, socket_w);
            continue;
          }
          printf("%stest \n", input->data[0]);
          if (strcmp(input->data[0],"BEG\n") !=0){
              tag = 1;
              args_len = 0;
              nbRessources = atoi(input->data[1]);
              nbClients = atoi(input->data[2]);  
              available = malloc (nbRessources * sizeof(int));
              clientWaiting = calloc(nbClients,sizeof(bool));
              for (int i = 0; i < nbClients; ++i){
              	clientWaiting[i] = false;
              }
          }

      //PRO
      }else if (command==2){
          if(array_get_size(input) != nbRessources + 1){
            sendErreur("Trop ou pas assez d'arguments(PRO res1 res2 ...)",socket_w);
            freeValues(args, input);
            closeStream(socket_r, socket_w);
            continue;
          }
          if (strcmp(input->data[0],"PRO") == 0){
                tag = 1;
                for (int i=0;i<nbRessources;i++){
                    available[i] = atoi(input->data[i+1]);
                }
      
                allocated = malloc (nbClients * sizeof (int*));
                max = malloc (nbClients * sizeof (int*));
                need = malloc (nbClients * sizeof (int*));

                for (int i=0; i<nbClients; i++){
                    allocated[i] = malloc (nbRessources * sizeof (int));
                    max[i] = malloc (nbRessources * sizeof (int));
                    need[i] = malloc (nbRessources * sizeof (int));
                }

                fillMatrix();
          }
      }
    }
  }while (!tag);
  sendAck(socket_w,-1);
  if (input) {delete_array_string(input);}
  closeStream(socket_r, socket_w);
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

  int end_time = time (NULL) + max_wait_time; //temps que j'attends 

  while(thread_socket_fd < 0 && accepting_connections) {//tant que je n'ai pas de requetes et que j'accepte les connexions

    thread_socket_fd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
  //http://pubs.opengroup.org/onlinepubs/009695399/functions/accept.html
  //Upon successful completion, accept() shall return the non-negative file descriptor of the accepted socket. Otherwise, -1 shall be returned and errno set to indicate the error.

    if (time(NULL) >= end_time) {//si j'ai depasse ou egal le temps que de fin d'attente
      break;
    }
  }
  return thread_socket_fd;   //si -1 pas eu 
}

void unlockAndDestroy(pthread_mutex_t mut){
    pthread_mutex_unlock(&mut);
    pthread_mutex_destroy(&mut);
}

void lockUnlockDestroy(pthread_mutex_t mut){
    pthread_mutex_lock(&mut);
    unlockAndDestroy(mut);
}

bool commEND (FILE *socket_r,FILE *socket_w){
      pthread_mutex_lock(&lockNbClient);
      pthread_mutex_lock(&lockCouDispa);
      if (nbClients==count_dispatched){
        pthread_mutex_lock(&locknbChaqRess);
        pthread_mutex_lock(&lockResLibres);
       
        for (int i = 0; i < nbRessources; ++i){

          if(nbChaqueRess[i] != available[i]){
            sendErreur("des ressources n'ont pas ete liberer",socket_w);
            
            return false ;
          }
        }

        sigint_handler(1);
        free(available);
        free(nbChaqueRess);
        
        //detruit les mutex et libère la mémoire pour mettre fin au serveur
        
        unlockAndDestroy(lockResLibres);
        unlockAndDestroy(locknbChaqRess);


        pthread_mutex_lock(&lockStrTock);
        unlockAndDestroy(lockStrTock);

        //pthread_mutex_lock(&lockClientWait);
        //delete_array(&clientQuiWait);
        //unlockAndDestroy(lockClientWait);
        /*
        pthread_mutex_lock(&lockMax);
        delete_array(&max);

        pthread_mutex_unlock(&lockMax);
        pthread_mutex_destroy(&lockMax);

        pthread_mutex_lock(&lockBesoin);
        delete_array(&besoin);
        pthread_mutex_unlock(&lockBesoin);
        pthread_mutex_destroy(&lockBesoin);

        pthread_mutex_lock(&lockAllouer);
        delete_array(&allouer);
        pthread_mutex_unlock(&lockAllouer);
        pthread_mutex_destroy(&lockAllouer);
        */
     
        unlockAndDestroy(lockNbClient);
        lockUnlockDestroy(lockCountAccep);        
        lockUnlockDestroy(lockCouWait);
        lockUnlockDestroy(lockCouInvalid);
        lockUnlockDestroy(lockCouDispa);
        lockUnlockDestroy(lockReqPro);
        lockUnlockDestroy(lockClientEnd);
        lockUnlockDestroy(lockClientWait);
        return true; 
      
    }
    sendErreur("Il reste des clients \n",socket_w);
    return false;
}


void st_process_requests (server_thread * st, int socket_fd){
  FILE *socket_r = fdopen (socket_fd, "r");
  FILE *socket_w = fdopen (socket_fd, "w");

  while (true){
    
    char *args = NULL; size_t args_len = 0;
    printf("About to getline Client dans process et socket_fd= %d \n", socket_fd);
    if(getline(&args,&args_len,socket_r) == -1){
      sendErreur("Pas reçu de commande",socket_w);
      if (args) {free(args);}
      break;
    }
	printf("Serveur a recu : %s  sur  %d",args,socket_fd);
    struct array_t_string *input= parseInput(args);
    printf(" du client %d \n",atoi(input->data[1]));

    if(strcmp(input->data[0],"END") == 0){
      printf("Serveur a reçu un END \n");
      commEND(socket_r,socket_w);
      if (input) {delete_array_string(input);}
      if (args) {free(args);}
      break;
    }
    else if(array_get_size(input)  < 2 ){
        sendErreur("Pas assez d'arguments",socket_w);
        if (input) {delete_array_string(input);}
        if (args) {free(args);}
        fclose (socket_r);
        fclose (socket_w);
        return;

    }else if( strcmp(input->data[0],"INI") == 0){

        int tidClient = atoi(input->data[1]);
        pthread_mutex_lock(&lockMax);
       for (int i=0; i<nbRessources;i++){
           if (atoi(input->data[i+2]) > available[i]){
                printf("Demande trop grande \n");
                fclose (socket_r);
                fclose (socket_w);
                return;
           }else{
                max[tidClient][i] = atoi(input->data[i+2]); 
                need[tidClient][i] = max[tidClient][i]; 
            }
        }

       pthread_mutex_unlock(&lockMax);
       sendAck(socket_w,tidClient);
       break;

    }else if (strcmp(input->data[0],"REQ") == 0){
      //printf("Serveur rentre dans le REQ \n");
      int tidClient = atoi(input->data[1]);  
      st_banker(tidClient,input, socket_w);

      if (args) {free(args);}
      if (input) {delete_array_string(input);}
      break;
    }

    else if(strcmp(input->data[0],"CLO") == 0){

      int tidClient = atoi(input->data[1]);
      for(int i=0;i<nbRessources;i++){
            if (allocated[tidClient][i]!=0){
                printf("Erreur, avant de close doit avoir libéré tout");
                fclose (socket_r);
                fclose (socket_w);
                return;
            }  
      }
    lockIncrementUnlock(lockClientEnd,clients_ended);
    lockIncrementUnlock(lockCouDispa,count_dispatched);
    sendAck(socket_w,tidClient);
    }else{
      sendErreur("ERR commande inconnu",socket_w); 
      if (input) {delete_array_string(input);}
      if (args) {free(args);}
      break;  
    }        

  }
fclose (socket_r);
fclose (socket_w);

}



void *st_code (void *param){
  server_thread *st = (server_thread *) param;

  int thread_socket_fd = -1;

  // Boucle de traitement des requêtes.
  while (accepting_connections)
  {
    // Wait for a I/O socket.
    thread_socket_fd = st_wait();
    //printf("Serveur a accepté le client FD %d \n", thread_socket_fd);
    if (thread_socket_fd < 0)
    {
      fprintf (stderr, "Time out on thread %d.\n", st->id);//reesaye plus tard
      continue;
    }

    if (thread_socket_fd > 0)//si j'ai eu une requete
    {
 
      //printf("Serveur va process la requête du FD %d \n", thread_socket_fd);
      st_process_requests (st, thread_socket_fd);

      close (thread_socket_fd);
    }
  }
  //printf("fin de st_code\n");
  return NULL;
}

void cleanBanker(struct array_t_string *input, FILE* socket_w){
    if (input) {delete_array_string(input);}
    fclose(socket_w);

}

//Algo inspiré de geeksforgeeks.org/operating-system-bankers-algorithm
int stateSafe(struct array_t_string *input,int tidClient){
    printf("Banquier vérifie l'état.. \n");
    int work[nbRessources];
    //TODO
    int finish[nbClients];
    for (int i = 0 ; i < nbClients; i++){
        finish[i] = false;
    }
    for (int i = 0; i < nbRessources; i++){
        work[i] = available[i];
        //available
    }
    int safeTag = true;
    int finishedTag = true;
    //Cherche client tq pas déjà fait (finish = false) et chaque need <= work(available)
    while(!finishedTag && safeTag){
        finishedTag = true;
        safeTag = true;
        for(int i = 0 ; i < nbClients ; i ++){
            if (!finish[i]){
                //Si toutes ressources dispos
                for (int j = 0; j < nbRessources; j++){
                    if (!((max[tidClient][i]-allocated[tidClient][i]) <= work[j])){ //besoins          
                        safeTag = false;
                        break;
                    }
                }
                if (safeTag){
                    for (int k = 0; k < nbRessources; k++){
                        work[k] += allocated[tidClient][k];
                    }
                    finish[i] = true;
                    i=0;
                }
            }
        }
        for (int l = 0; l < nbClients ; l ++){
            if (!finish[l]){
                finishedTag = false;
            }
        }
    }
    //Si on est instable, on n'a pas choisi le bon truc
    if (!safeTag){
        for (int k=0; k<nbRessources;k++){
            //On efface ce qu'on avait fait
            available[k] += atoi(input->data[k+2]);
            allocated[tidClient][k] -= atoi(input->data[k+2]);
            need[tidClient][k]  += atoi(input->data[k+2]);
        }
        return 0;
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

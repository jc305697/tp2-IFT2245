//#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */
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
#include "dyn_array.h"

void flushmoica();



enum { NUL = '\0' };

enum {
  /* Configuration constants.  */
  max_wait_time = 30,
  server_backlog_size = 5
};


pthread_mutex_t lockNbClient;
pthread_mutex_t lockResLibres;
pthread_mutex_t lockMax;
pthread_mutex_t lockAllouer;
pthread_mutex_t lockCountAccep;//nombre de requete accepter
pthread_mutex_t lockCouWait;//nombre de requete accepter avec mise en attente
pthread_mutex_t lockCouInvalid;//nombre de requete erronees
pthread_mutex_t lockCouDispa;//nombre de clients qui se sont terminés correctement
pthread_mutex_t lockReqPro;//nombre total de requête traites
pthread_mutex_t lockClientEnd;//nombre de clients ayant envoye le message CLO
pthread_mutex_t lockClientWait;//Clients a qui j'ai dit de wait 
pthread_mutex_t lockBesoin; 
pthread_mutex_t locknbChaqRess;
pthread_mutex_t lockBanker;
pthread_mutex_t lockStrTock;


int server_socket_fd;

struct Client{
  int tid;
  int *ressClient;
};
// Nombre de client enregistré.
int nb_registered_clients;

int *ressourcesLibres;
int *nbChaqueRess;
int nbRessources;

void  attendBeg( socklen_t socket_len );
void attendPro( socklen_t socket_len);

struct array_t clientQuiWait; //Clients a qui j'ai dit de wait 
struct array_t max;
struct array_t allouer;
struct array_t besoin;

struct sockaddr_in thread_addr;
// Variable du journal.
// Nombre de requêtes acceptées immédiatement (ACK envoyé en réponse à REQ).
unsigned int count_accepted = 0;

int nbClients = 0;
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

struct array_t {
  size_t size, capacity;
  struct Client **data;
};

struct array_t_string{
  size_t size, capacity;
  char **data;
};

struct array_t *new_array (size_t capacity) {
  struct array_t *newA = malloc(sizeof(*newA));
  if (!newA) {//erreur dans l'allocation de la mémoire
    errno = ENOMEM;
    return NULL;
  }

    newA->capacity = capacity;
    newA->size = 0;

    newA->data = malloc(capacity*sizeof(struct Client*));
  if(!newA->data) {
    free(newA);
      newA = NULL;
    errno = ENOMEM;
  }
  return newA;
}

struct array_t_string *new_arrayString (size_t capacity) {
  struct array_t_string *newA = malloc(sizeof(*newA));
  if (!newA) {
    errno = ENOMEM;
    return NULL;
  }

    newA->capacity = capacity;
    newA->size = 0;
    newA->data = malloc(capacity*sizeof(char *));

  if(!newA->data) {
    free(newA);
      newA = NULL;
    errno = ENOMEM;
  }
  return newA;
}

int push_back(struct array_t *array, struct Client *element) {
  if (array->size > (array->capacity - 2)) {
    size_t newsize = array->capacity << 1;
    struct Client **tmp = (struct Client **)realloc(array->data, newsize*sizeof(struct Client*));
    if (!tmp) {
      errno = ENOMEM;
      return -1;
    }
    array->capacity = newsize;
    //array->data = &tmp;
    array->data = tmp;
  }
  array->data[array->size] = element;
  array->size++;
  return 0; 
}

int push_backString(struct array_t_string *array, char *element) {
  if (array->size > (array->capacity - 2)) {
    //printf("realloue dans push_backString\n");
    size_t newsize = array->capacity << 1;
    //printf("capacity augmenter dans push_backString\n");
    char  **tmp = (char **)realloc(array->data, newsize*sizeof(char *));
   
    if (!tmp) {
      perror("pushBackString");
      errno = ENOMEM;
      return -1;
    }
    //printf("realloc reussi dans push_backString\n");
    array->capacity = newsize;
    //printf("capacity mis a jour dans push_backString\n");   
    //array->data = &tmp;
    array->data = tmp;
    //printf(" array->data mis a jour dans push_backString\n"); 
  }


  array->data[array->size] = element;
  //printf("Ajouter element dans push_backString\n");
  array->size++;
  return 0; 
}

void delete_array (struct array_t *array) {
  if(array) {
    struct array_t *ptr; 
    if ((ptr = array)) {
      free(ptr->data);
      free(ptr);
    }
    array = NULL;
  }
}

void delete_array_string (struct array_t_string *array) {
  if(array) {//si array n'est pas NULL 
      free(array->data);
      free(array);
      array = NULL; 
  }
}

struct array_t deleteClientInArray(struct array_t *array, int clientTid){
  struct array_t arrayTemp = *new_array(array->size);
  for (int i = 0; i < array->size; ++i){
    struct Client *listeClient = *(*array).data;
    if(listeClient[i].tid!=clientTid){
      push_back(&arrayTemp,&(*(*array).data)[i]);
    }
  }
  return arrayTemp;
}

void sendErreur(const char *message, int socket_fd){
  //envoie au client le message d'erreur 
  printf("Serveur va envoyer ERR %s \n",message);
  char mymessage[25];
  sprintf(mymessage,"ERR %s",message); //
  if (send (socket_fd, mymessage, strlen(mymessage),0)<0){
      printf("Send failed");
      return;
  }
  pthread_mutex_lock(&lockCouInvalid);
  count_invalid = count_invalid + 1;
  pthread_mutex_unlock(&lockCouInvalid); 
  pthread_mutex_lock(&lockReqPro);
  request_processed = request_processed + 1;
  pthread_mutex_unlock(&lockReqPro);
  return;
}

void sendWait(int temps,int socket_fd, struct Client client){
 printf("Serveur va envoyer WAIT %d \n", temps);
 char mymessage[25];
 sprintf(mymessage,"WAIT %d",temps); //
 if (send (socket_fd, mymessage, strlen(mymessage),0)<0){
      printf("Send failed");
      return;
 }
 pthread_mutex_lock(&lockClientWait);
 push_back(&clientQuiWait,&client);
 pthread_mutex_unlock(&lockClientWait);
 pthread_mutex_lock(&lockReqPro);
 request_processed = request_processed + 1;
 pthread_mutex_unlock(&lockReqPro);
 return;
}

bool estDansListe(struct array_t *liste,int clientTid ){
  if(liste){
    for (int i = 0; i < (*liste).size; ++i){

      if((liste->data)[i]->tid==clientTid){
        return true;
      }
    }
    return false;
  }

  return false;
}


void sendAck(int socket_fd){
int clientTid = -1;
//TODO
 char mymessage[25] = "ACK \n";
 if (send (socket_fd, mymessage, strlen(mymessage),0)<0){
      printf("Send failed");
      return;
 }
  
  pthread_mutex_lock(&lockCountAccep);
  count_accepted = count_accepted + 1;
  pthread_mutex_unlock(&lockCountAccep);
  pthread_mutex_lock(&lockClientWait);
  if ( clientTid != -1 &&estDansListe(&clientQuiWait,clientTid)){
    pthread_mutex_lock(&lockCouWait);
    count_wait = count_wait + 1;
    pthread_mutex_unlock(&lockCouWait);
    deleteClientInArray(&clientQuiWait,clientTid);
  }
  pthread_mutex_unlock(&lockClientWait);
  pthread_mutex_lock(&lockReqPro);
  request_processed = request_processed + 1;
  pthread_mutex_unlock(&lockReqPro);
  return;
}

struct array_t_string *parseInput(char *input){
  pthread_mutex_lock(&lockStrTock);
  char *token =strtok(input,"\n");
  pthread_mutex_unlock(&lockStrTock);

  struct array_t_string *array = new_arrayString(5);

  pthread_mutex_lock(&lockStrTock);
  token = strtok(token," ");
  
  int i =0;

  while(token != NULL){
  	if(push_backString(array,token)==-1){
  		perror("parseinput error");
  	}
  	token = strtok(NULL," ");
  	 i +=1;
  }
  pthread_mutex_unlock(&lockStrTock);
  return array;
}

void freeValues(char *args, struct array_t_string *input){
    free(args);
    delete_array_string(input);
}
void attendBeg( socklen_t socket_len ){
  int socket_fd = -1;
  bool bonneCommande = false;

  printf("Server dans le BEG \n");
  while(!bonneCommande){
    while( socket_fd == -1) { 
      // attend le beg 
      socket_fd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
    }
    nbClients+=1;

    printf("About to getline \n");
    char message[50];
    if (recv (socket_fd, message, 50, 0 )<0){
        printf("Receive failed");
        continue;
    

    }else{
      struct array_t_string *input = parseInput(message);
      if(input->size != 2 ){
        sendErreur("trop d'arguments",socket_fd);
        delete_array_string(input); 
        continue;
      }

      if( strcmp(input->data[0],"BEG") == 0){
          //commande est beg
        int valeur = atoi(input->data[1]);
      
        if (valeur == 0 && strcmp(input->data[1],"0") != 0){
            sendErreur("premier argument pas un int",socket_fd);         
            delete_array_string(input); 
            continue;
       } else if (valeur==0){
            sendErreur("le nombre de ressource ne peut pas etre egale a 0",socket_fd);
            delete_array_string(input); 
            continue;
       } else if (valeur<0){
            sendErreur("le nombre de ressource ne peut pas etre inferieure à 0",socket_fd);
            delete_array_string(input); 
            continue;
       } else{
            printf("SERVEUR A BIEN REÇU LE BEG \n");

            ressourcesLibres = calloc(valeur,sizeof(int));
            nbChaqueRess = calloc(valeur,sizeof(int));
            nbRessources = valeur;
            sendAck(socket_fd); //need client id?
            delete_array_string(input);
            break;
       }
      }else{
           sendErreur("mauvaise commande, on attend BEG", socket_fd);
           delete_array_string(input); 
     } 
    }
  }
  return;
}

void attendPro(socklen_t socket_len){
  int socket_fd = -1;
  bool bonneCommande = false;
  printf("Server attend le pro \n");
  while(!bonneCommande){
    while( socket_fd == -1) { 
      socket_fd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
    }
    
     char message[50];
    if (recv (socket_fd, message, 50, 0 )<0){
        printf("Receive failed");
        continue;
    }

    else{
      printf("Right before parseinputgetline \n");
      struct array_t_string *input = parseInput(message);
      
      if(input->size != nbRessources + 1){
        printf("ERR trop d'arguments");
delete_array_string(input); 
        continue;
      }
      printf("After the argument check \n");
      if( strcmp(input->data[0],"PRO") == 0){
        //commande est PRO
        printf("Serveur a reçu un PRO \n ");
        int longueur = 0; 
        while(longueur != nbRessources){
        int valeur;
        longueur = longueur + 1;

        while(longueur != nbRessources){//je verifie chacune des ressoruces 

          valeur = atoi(input->data[longueur]);

          if (valeur == 0 && strcmp(input->data[longueur],"0") != 0){

            printf("ERR une argument n'est pas un int");
                   delete_array_string(input); 
            continue;
          }

          else if (valeur==0){//une resssource aura un nb de ressource 
            printf("ERR le nombre de ressource ne peut pas etre egale a 0 ... (gros jugement)");
                    delete_array_string(input); 
              continue;
          }

          else{
             ressourcesLibres[longueur - 1] = valeur;
             nbChaqueRess[longueur - 1] = valeur;
             longueur = longueur + 1;

             if (longueur == nbRessources){
               bonneCommande = true;
               sendAck(socket_fd);
               break;
             }
          }
        }
       }
      }
     else{
      printf("ERR mauvais commande attend PRO");
     }
    }
  } 
}



static void sigint_handler(int signum){
  // Code terminaison.
  printf("je recois un signal d'interruption\n");
  accepting_connections = 0;//je n'acccepte plus de connection
}

void st_init (){
  // Handle interrupt
  printf("Serveur dans le init \n");
  signal(SIGINT, &sigint_handler); 
  // Initialise le nombre de clients connecté.
  nb_registered_clients = 0;

  int retour_init;

  retour_init = pthread_mutex_init(&lockStrTock,NULL);//initialise le mutex
  if (retour_init != 0){
    perror("ERR erreur init mutex nombre de client");//send au client ?
  }


  retour_init = pthread_mutex_init(&lockNbClient,NULL);//initialise le mutex
  if (retour_init != 0){
    perror("ERR erreur init mutex nombre de client");//send au client ?
  }

   retour_init = pthread_mutex_init(&lockResLibres,NULL);//initialise le mutex
   if (retour_init != 0){
    perror("ERR erreur init mutex nombre de ressources libres");//send au client ?
  }

   retour_init = pthread_mutex_init(&lockMax,NULL);//initialise le mutex
   if (retour_init != 0){
    perror("ERR erreur init mutex nombre de ressource maximum");//send au client ?
  }

   retour_init = pthread_mutex_init(&lockAllouer,NULL);//initialise le mutex
   if (retour_init != 0){
    perror("ERR erreur init mutex nombre de ressource allouer");//send au client ?
  }


  retour_init = pthread_mutex_init(&lockCountAccep,NULL);//initialise le mutex
  if (retour_init != 0){
    perror("ERR erreur init mutex nombre de requete accepter ");//send au client ?
  }

  retour_init = pthread_mutex_init(&lockCouWait,NULL);//initialise le mutex
  if (retour_init != 0){
    perror("ERR erreur init mutex nombre de requete accepter avec mise en attente ");//send au client ?
  }

  retour_init = pthread_mutex_init(&lockCouInvalid,NULL);//initialise le mutex
  if (retour_init != 0){
    perror("ERR erreur init mutex nombre de requete erronees");//send au client ?
  }

  retour_init = pthread_mutex_init(&lockCouDispa,NULL);//initialise le mutex
  
  if (retour_init != 0){
    perror("ERR erreur init mutex nombre de clients qui se sont terminés correctement");//send au client ?
  }

  retour_init = pthread_mutex_init(&lockReqPro,NULL);//initialise le mutex
  
  if (retour_init != 0){
    perror("ERR erreur init mutex nombre total de requête traites");//send au client ?
  }

  retour_init = pthread_mutex_init(&lockClientEnd,NULL);//initialise le mutex
  
  if (retour_init != 0){
    perror("ERR erreur init mutex nombre de clients ayant envoye le message CLO.");//send au client ?
  }

  retour_init = pthread_mutex_init(&lockBesoin,NULL);//initialise le mutex
  
  if (retour_init != 0){
    perror("ERR erreur init mutex tableau pour les besoins des clients.");//send au client ?
  }

  
  retour_init = pthread_mutex_init(&locknbChaqRess,NULL);//initialise le mutex
  
  if (retour_init != 0){
    perror("ERR erreur init mutex pour tableau nombre total de ressource de chaque type.");//send au client ?
  }
  max = *new_array(5);
  clientQuiWait = *new_array(5);
  besoin = *new_array(5);
  allouer = *new_array(5);

  socklen_t socket_len = sizeof (thread_addr);

  attendBeg(socket_len);
  //printf("attendBeg fini\n");
  attendPro(socket_len);
  printf("attendPro fini\n");


  // TODO

  // Attend la connection d'un client et initialise les structures pour
  // l'algorithme du banquier.

  // END TODO
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


bool commEND (int socket_fd){
      pthread_mutex_lock(&lockNbClient);
      pthread_mutex_lock(&lockCouDispa);
      if (nb_registered_clients==count_dispatched){
        pthread_mutex_lock(&locknbChaqRess);
        pthread_mutex_lock(&lockResLibres);
       
        for (int i = 0; i < nbRessources; ++i){

          if(nbChaqueRess[i] != ressourcesLibres[i]){
            printf("ERR des ressources n'ont pas ete liberer");
            return false ;
          }
        }

        sigint_handler(1);
        free(ressourcesLibres);
        free(nbChaqueRess);
        
        //detruit les mutex et libère la mémoire pour mettre fin au serveur
        
        pthread_mutex_unlock(&lockResLibres);
        pthread_mutex_destroy(&lockResLibres);
        pthread_mutex_unlock(&locknbChaqRess);
        pthread_mutex_destroy(&locknbChaqRess);

        pthread_mutex_lock(&lockStrTock);
        pthread_mutex_unlock(&lockStrTock);
        pthread_mutex_destroy(&lockStrTock);

        pthread_mutex_lock(&lockClientWait);
        delete_array(&clientQuiWait);
        pthread_mutex_unlock(&lockClientWait);
        pthread_mutex_destroy(&lockClientWait);
        
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
        
       // pthread_mutex_lock(&lockNbClient);//lock et unlock afin  de s'assurer que on detruit pas un mutex qui est utiliser
        pthread_mutex_unlock(&lockNbClient);
        pthread_mutex_destroy(&lockNbClient);

        pthread_mutex_lock(&lockCountAccep);//lock et unlock afin  de s'assurer que on detruit pas un mutex qui est utiliser
        pthread_mutex_unlock(&lockCountAccep);
        pthread_mutex_destroy(&lockCountAccep);
        
        pthread_mutex_lock(&lockCouWait);//lock et unlock afin  de s'assurer que on detruit pas un mutex qui est utiliser
        pthread_mutex_unlock(&lockCouWait);
        pthread_mutex_destroy(&lockCouWait);

        pthread_mutex_lock(&lockCouInvalid);//lock et unlock afin  de s'assurer que on detruit pas un mutex qui est utiliser
        pthread_mutex_unlock(&lockCouInvalid);
        pthread_mutex_destroy(&lockCouInvalid);

        pthread_mutex_lock(&lockCouDispa);//lock et unlock afin  de s'assurer que on detruit pas un mutex qui est utiliser
        pthread_mutex_unlock(&lockCouDispa);
        pthread_mutex_destroy(&lockCouDispa);
        
        pthread_mutex_lock(&lockReqPro);//lock et unlock afin  de s'assurer que on detruit pas un mutex qui est utiliser
        pthread_mutex_unlock(&lockReqPro);
        pthread_mutex_destroy(&lockReqPro);
        
        pthread_mutex_lock(&lockClientEnd);//lock et unlock afin  de s'assurer que on detruit pas un mutex qui est utiliser
        pthread_mutex_unlock(&lockClientEnd);
        pthread_mutex_destroy(&lockClientEnd);

        pthread_mutex_lock(&lockClientWait);//lock et unlock afin  de s'assurer que on detruit pas un mutex qui est utiliser
        pthread_mutex_unlock(&lockClientWait);
        pthread_mutex_destroy(&lockClientWait);
       return true; 
      }

      else{
        sendErreur("il reste des clients \n",socket_fd);
        return false;
      }
}

bool verifiePremierArgs (struct array_t_string *args, int socket_fd){
        if (args->size < 2 ){
          sendErreur("pas assez d'arguments",socket_fd);
          return false;
        }        
        int valeur = atoi(args->data[1]);

        if (valeur == 0 && strcmp(args->data[1],"0") != 0){
          sendErreur("tid n'est pas un int",socket_fd); 
            printf("ERR tid n'est pas un int");
          return false;
        }
        return true;
}

bool checkInt(struct array_t_string *input, int valeur, int longueur, int socket_fd){
      if ( valeur == 0 && strcmp(input->data[longueur],"0") != 0){
        sendErreur("erreur une valeur n'est pas un int",socket_fd);
        //printf("ERR erreur une valeur n'est pas un int"); 
        delete_array_string(input);
        return false;
      }
      return true;
}

void myfree(struct array_t_string *input, int* ressourcestemp){
   delete_array_string(input); 
   free(ressourcestemp);
}

void checkIni(int longueur, int tidClient, struct array_t_string *input, int* ressourcestemp, int socket_fd){
    while(longueur != input->size){
              int  valeur = atoi(input->data[longueur]);

              if(!checkInt(input, valeur, longueur, valeur)){
                   myfree(input, ressourcestemp);
                   return;
              }

              if (valeur<0){
                sendErreur("erreur une valeur est negative",socket_fd);
                printf("ERR erreur une valeur est negative"); 
                myfree(input, ressourcestemp);
                return;
              }
              ressourcestemp[longueur-2] = valeur;
              longueur = longueur + 1;
            } 
            if (longueur-2 != nbRessources){//les ressources n'ont pas tous été déclaré
                sendErreur("mauvais nombre de ressources specifier",socket_fd);
                printf("ERR mauvais nombre de ressources specifier");
                myfree(input, ressourcestemp);
                return;
            }          
           struct Client nouvClient = {tidClient,ressourcestemp};
           pthread_mutex_lock(&lockMax);

           if (push_back(&max,&nouvClient) == -1){
            sendErreur("erreur interne",socket_fd);
            printf("erreur interne");
            pthread_mutex_unlock(&lockMax);
            myfree(input, ressourcestemp);
            return -1;
           }

           pthread_mutex_unlock(&lockMax);
           int *besoinTemp = calloc(nbRessources,sizeof(int));

           for (int i = 0; i < nbRessources; ++i){
             besoinTemp[i] = ressourcestemp[i]; //initialise le tableau des besoins 
           }                                     //le client a besoin du maximum

           struct Client nouvClientBesoin = {tidClient,besoinTemp};
           pthread_mutex_lock(&lockBesoin);
           if (push_back(&besoin,&nouvClientBesoin) == -1){
             sendErreur("erreur interne",socket_fd);
            printf("ERR interne");         
            pthread_mutex_unlock(&lockBesoin);
            free(ressourcestemp);
            return;
           }
           pthread_mutex_unlock(&lockBesoin);
           pthread_mutex_lock(&lockNbClient);
           nb_registered_clients = nb_registered_clients + 1;  //j'ai un client de plus 
           pthread_mutex_unlock(&lockNbClient);
           sendAck(socket_fd);
           return;
        
}

void checkReq(int longueur, int tidClient, struct array_t_string *input, int* ressourcesDem, int socket_fd){
    int i = 0;
     while(longueur + 2 != input->size){
        int  valeur = atoi(input->data[longueur]);
        if ( valeur == 0 && strcmp(input->data[i],"0") != 0){
          sendErreur("erreur une valeur n'est pas un int",socket_fd);
          myfree(input,ressourcesDem);
          return;
        }
        else{
          ressourcesDem[longueur] = valeur;
          longueur = longueur + 1;
        }    
        i++;   
      }

      if (longueur-2 != nbRessources){//les ressoruces n'ont pas tous été déclaré
         sendErreur("mauvais nombre de ressources specifier",socket_fd);
          myfree(input,ressourcesDem);
       }
      pthread_mutex_lock(&lockBesoin);

      int j = 0;

      while(j!= besoin.size){
        if ((*besoin.data)[j].tid == tidClient){
          break;
        }

        j = j + 1;
      }

      if (j==nbRessources){
        printf("ERR le client n'a pas ete initiliase");   
        pthread_mutex_unlock(&lockBesoin);
            myfree(input,ressourcesDem);
      }

      int *ressBesoinClient = (*besoin.data)[j].ressClient;

      pthread_mutex_lock(&lockResLibres);
      int assezRessPourMax = 0;
      for (int i = 0; i < nbRessources; ++i){
        //je regarde si j'ai assez de ressources de chaque pour allouer au client 
        //et si j'ai une erreur puisque demande plus qu'il en a besoin 
        if(ressourcesDem[i] > ressourcesLibres[i]){
          sendWait(max_wait_time,socket_fd,*max.data[j]);
          pthread_mutex_unlock(&lockResLibres); 
          pthread_mutex_unlock(&lockBesoin);
          myfree(input,ressourcesDem);
          return;
        }

        else if(ressourcesDem[i]>ressBesoinClient[i]){
          printf("ERR client demande plus de ressources que le max declarer dans ini");
          
          pthread_mutex_unlock(&lockResLibres); 
          pthread_mutex_unlock(&lockBesoin);
           myfree(input,ressourcesDem);
          //closeStream(socket_r, socket_w);
          return;
        }

        else if(ressBesoinClient[i]<=ressourcesLibres[i]){
          assezRessPourMax = assezRessPourMax + 1;
        } 
      }

      pthread_mutex_lock(&lockAllouer);


      int *ressAllouerClient = (*allouer.data)[j].ressClient;

      if (assezRessPourMax == nbRessources){
       
        for (int i = 0; i < nbRessources; ++i){
         ressAllouerClient[i] = ressAllouerClient[i] + ressourcesDem[i];
         ressourcesLibres[i] =  ressourcesLibres[i] - ressBesoinClient[i];
         ressBesoinClient[i]= ressBesoinClient[i] - ressBesoinClient[i];
        }
      }

      else{
        for (int i = 0; i < nbRessources; ++i){
          ressAllouerClient[i] = ressAllouerClient[i] + ressourcesDem[i];
          ressourcesLibres[i] =  ressourcesLibres[i] - ressourcesDem[i];
          ressBesoinClient[i]= ressBesoinClient[i] - ressourcesDem[i];
        }
      }

      pthread_mutex_unlock(&lockAllouer);
      pthread_mutex_unlock(&lockResLibres); 
      pthread_mutex_unlock(&lockBesoin);
          myfree(input,ressourcesDem);
}

int st_process_requests (server_thread * st, int socket_fd){
    char message[25];

    while (true){
    printf("Server %d about to receive process et socket_fd = %d \n",st->id, socket_fd);

    if (recv (socket_fd, message, 50,0)<0){
        printf("ERR mauvaise commande");
        return -1;
    }
    printf("Server %d a recu : %s \n", st->id, message);
    size_t msize = sizeof(message);
    char copy[msize];
    strncpy(copy, message, msize);
    struct array_t_string *input= parseInput(message);
    printf("du client %d sur le FD %d \n",input->data[1],socket_fd);


	printf("Server %d a recu : %s du client %d sur le FD %d \n", st->id, copy,input->data[1],socket_fd);
    if(strcmp(input->data[0],"END") == 0){
      printf("Ending server..");
      delete_array_string(input);
      return -2;
    }

    if(!verifiePremierArgs(input, socket_fd)){
        delete_array_string(input); 
        return -1; 
    }

    if( strcmp(input->data[0],"INI") == 0){
        printf("rentre dans INI Server %d Client %d FD %d \n", st->id, socket_fd);
        int *ressourcestemp = calloc(nbRessources, sizeof(int));
        int tidClient = atoi(input->data[1]);
        int longueur = 2;
        checkIni(longueur,tidClient, input,ressourcestemp, socket_fd);
        
    }
    else if (strcmp(input->data[0],"REQ") == 0){
      int *ressourcesDem = calloc(nbRessources, sizeof(int));
      int tidClient = atoi(input->data[1]);
      int longueur = 2;
      checkReq(longueur, tidClient, input, ressourcesDem, socket_fd);
 
    }

    else if(strcmp(input->data[0],"CLO") == 0){

      pthread_mutex_lock(&lockClientEnd);
      clients_ended += 1;
      pthread_mutex_unlock(&lockClientEnd);      


      int tidClient = atoi(input->data[1]);
      pthread_mutex_lock(&lockAllouer);
      int positionAllouer = 0;

      while(positionAllouer!= (allouer.size - 1) ){//je cherche la position du client concerné dans l'array
        if ((*allouer.data)[positionAllouer].tid == tidClient){
          break;
        }

        positionAllouer = positionAllouer + 1;
      }

      if (positionAllouer == nbRessources){
        printf("ERR le client n'a pas ete initiliase");   
        pthread_mutex_unlock(&lockAllouer);
        break;
      }

      pthread_mutex_lock(&lockResLibres);
      int ressource = 0;
      while(ressource != nbRessources){//je rajoute les ressources allouer aux ressources lbres
        ressourcesLibres[ressource] += (*allouer.data)[positionAllouer].ressClient[ressource];
      }
      pthread_mutex_unlock(&lockResLibres);
      allouer = deleteClientInArray(&allouer,tidClient);
      pthread_mutex_unlock(&lockAllouer);
      pthread_mutex_lock(&lockBesoin);

      besoin = deleteClientInArray(&besoin,tidClient);

      pthread_mutex_unlock(&lockBesoin);

      pthread_mutex_lock(&lockMax);

      max = deleteClientInArray(&max,tidClient);

      pthread_mutex_unlock(&lockMax);

      pthread_mutex_lock(&lockCouDispa);
      //count_dispatched
      count_dispatched += 1;

      pthread_mutex_unlock(&lockCouDispa);

      sendAck(socket_fd);
    }

    else{
      printf("ERR commande inconnu"); 
      delete_array_string(input);
      break;  
    }        
  
}

//fclose (socket_r);
//fclose (socket_w);

}



void *st_code (void *param){
  server_thread *st = (server_thread *) param;

  int thread_socket_fd = -1;
  //printf("rentre dans stcode\n");
  // Boucle de traitement des requêtes.
  while (accepting_connections)
  {
    // Wait for a I/O socket.
    printf("Server %d commence le wait\n", st->id);
    thread_socket_fd = st_wait();
    printf("Server %d a accepté un client sur le FD  %d \n", st->id, thread_socket_fd);
    if (thread_socket_fd < 0)
    {
      fprintf (stderr, "Time out on thread %d.\n", st->id);//reesaye plus tard
      continue;
    }

    if (thread_socket_fd > 0)//si j'ai eu une requete
    {
      //printf("va rentrer dans st_process_requests \n");
      //printf(" \n");
      printf("Server %d va process la requête de Thread FD %d \n",st->id, thread_socket_fd);
      st_process_requests (st, thread_socket_fd);
      //printf("est sorti de st_process_requests \n");
      //printf(" \n");
      close (thread_socket_fd);
    }
  }
  printf("fin de st_code\n");
  return NULL;
}
/*
void st_banker(int client_id, array_t client_request){
pthread_mutex_lock(&lockBanker);
int assezRessources = 1;
int formatValide = 1;

//Algo inspiré de geeksforgeeks.org/operating-system-bankers-algorithm
for (i=0;formatValide && assezRessources && i<nbRessources;i++){
    //Requete demande n'est pas plus grande que le besoin
    formatValide = 
    client_request[i] <= arrayBesoinsClients[client_id][i] 
    && 
    //Requête libération n'est plus grande que ce qu'il a
    client_request[i] <= arrayAllouesClients[client_id][i]
    && 
    //Si un des éléments n'est pas valide, le tout ne l'est pas
    formatValide;

    assezRessources = 
    //On a assez pour traiter la requête
    client_request[i] <= ressourcesLibres[i]
    &&
    assezRessources;
} 

if (!formatValide){
    //ferme le lock
    pthread_mutex_unlock(&lockBanker);
    printf("BANKER - REQ DENIED FOR CLIENT %d \n",client_id);
    return;
}

if (!assezRessources){
    //ferme le lock
    pthread_mutex_unlock(&lockBanker);
    printf("BANKER - REQ MUST WAIT FOR CLIENT %d \n", client_id);
    //TODO:Appeler le wait
    return;
}

 //System prétend qu'il a alloué les ressources
for (int i=0; i<nbRessources;i++){
    ressourcesLibres[i] -= client_request[i];
    arrayAllouesClient[client_id][i] += client_request[i];
    arrayBesoinsClient[client_id][i] -= client_request[i];
}


}



void stateSafe(){
    int work[nbRessources];
    int finish[nbClients];
    for (int i = 0 ; i < nbClients; i++){
        finish[i] = false;
    }
    for (int i = 0; i < nbRessources; i++){
        work[i] = ressourcesLibres[i];
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
                    if (!arrayBesoinsClients[client_id][i] <= work[j]){ //besoins          
                    safeTag = false;

                }
                //On alloue
                if (safeTag){
                    for (int k = 0; k < nbRessources; k++){
                        work[j] += arrayAllouesClients[client_id][i] //allocation
                    }
                    finish[i] = true;
                    i=0;
                }
            }
        }
        for (int l = 0; l < nbClients ; l ++){
            if (!finish[i]){
                finishedTag = false;
            }
        }

    }
        //Si on est instable, on n'a pas choisi le bon truc
        if (!safeTag){
            //On efface ce qu'on avait fait
            ressourcesLibres[i] += client_request[i];
            arrayAllouesClient[client_id][i] -= client_request[i];
            arrayBesoinsClient[client_id][i] += client_request[i];
        }
        pthread_mutex_unlock(&lockBanker);
        return;
}
*/
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

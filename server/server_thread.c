//#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */
#include "server_thread.h"
//#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
#include<pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
//#include <stdbool.h>
//debut code pris de fred
#include "dyn_array.h"

//#include <errno.h>
//#include <malloc.h>
void flushmoica();

//fin premiere partie code de fred


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

//struct Client *allouer;
//struct Client *besoin;

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

//debut deuxieme partie code de fred
struct array_t {
  size_t size, capacity;
  struct Client **data;
 // void **data;
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

    //newA->data = malloc(capacity*sizeof(char**));
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
  //printf(" size mis a jour dans push_backString\n");  
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
    //struct array_t_string *ptr; 
    //if ((ptr = array)) {
      //printf("free le data\n");
      free(array->data);
      //printf("free le pointeur\n");
      free(array);
    //}
    array = NULL; 
  }
}

//fin deuxieme partie code de fred
struct array_t deleteClientInArray(struct array_t *array, int clientTid){
  //inspire par le code de fred

  struct array_t arrayTemp = *new_array(array->size);
  for (int i = 0; i < array->size; ++i){
    struct Client *listeClient = *(*array).data;
    if(listeClient[i].tid!=clientTid){
      push_back(&arrayTemp,&(*(*array).data)[i]);
    }
  }
  return arrayTemp;
}

void sendErreur(const char *message, FILE *socket_w){
  //envoie au client le message d'erreur 
  printf("Serveur va envoyer %s \n",message);
  fprintf (socket_w, "%s",message);
  fflush(socket_w);
   //printf("a envoyé %s \n",message);

  pthread_mutex_lock(&lockCouInvalid);
  count_invalid = count_invalid + 1;
  pthread_mutex_unlock(&lockCouInvalid); 
  pthread_mutex_lock(&lockReqPro);
  request_processed = request_processed + 1;
  pthread_mutex_unlock(&lockReqPro);
  return;
}

void sendWait(int temps,FILE *socket_w,struct Client client){
 printf("Serveur va envoyer Wait\n");
 fprintf (socket_w, "Wait %d",temps);
 fflush(socket_w);
 //printf("va envoyé Wait\n");

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

void imprimeArrayString(struct array_t_string *array){
  //printf("Contenu de input\n");
  for (int i = 0; i < array->size; ++i){
    printf("%s ",array->data[i] );
  }
  printf("\n");
  //fflush(stdout);
}


void sendAck(FILE *socket_w, int clientTid){
  printf("va envoyer ACK\n");
  fprintf (socket_w, "ACK \n");
  fflush(socket_w);
  //printf("a envoyé ACK\n");
  printf("\n" );
  
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

void flushmoica(){
  fflush(stdout);
}



struct array_t_string *parseInputGetLine(char *input){
  pthread_mutex_lock(&lockStrTock);
  char *token =strtok(input,"\n");
  pthread_mutex_unlock(&lockStrTock);

  struct array_t_string *array = new_arrayString(5);

  pthread_mutex_lock(&lockStrTock);
  token = strtok(token," ");
  
 /* if (token == NULL){
    return array;
  }*/
  int i =0;

  while(token != NULL){
  	//printf("ajoute a l'array iteration %d\n",i );
  	if(push_backString(array,token)==-1){
  		perror("parseinputgetline");
  	}
  	token = strtok(NULL," ");
  	 i +=1;
  }

  pthread_mutex_unlock(&lockStrTock);

  return array;
}

void closeStream(FILE *sockr, FILE *sockw){
    fclose(sockr);
    fclose(sockw);
}

void freeValues(char *args, struct array_t_string *input){
    free(args);
    delete_array_string(input);
}
void attendBeg( socklen_t socket_len ){
  int socketFd = -1;
  bool bonneCommande = false;

  printf("Server dans le BEG \n");
  while(!bonneCommande){
    while( socketFd == -1) { 
      // attend le beg 
      socketFd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
    }

    int retour = fcntl(socketFd,F_GETFL);
    //printf("%s\n", );
    fcntl(socketFd,F_SETFL,retour& ~O_NONBLOCK);

    
   /* printf("Server accepted connexion from : %s \n", &thread_addr);
    printf("%d\n",retour & O_NONBLOCK );
    printf("%d\n",O_NONBLOCK );*/
    
    FILE *socket_r = fdopen (socketFd, "r");
    FILE *socket_w = fdopen (socketFd, "w");

    //char *args = (char*) malloc (10*sizeof(char));
    char *args; 
    size_t args_len=0;

    printf("About to getline \n");
    if(getline(&args,&args_len,socket_r) == -1){
      printf("J'AI PAS REÇU UNE COMMANDE \n");
      perror("getline");
      sendErreur("ERR mauvaise commande",socket_w);
      fclose (socket_r);
      fclose (socket_w);
      continue;

    }else{//pas d'erreur avec getline 
      printf("Commande reçue \n");
      struct array_t_string *input = parseInputGetLine(args);
      imprimeArrayString(input);
      if(input->size != 2 ){
        sendErreur("ERR trop d'arguments",socket_w);
        freeValues(args, input);
        closeStream(socket_r, socket_w);
        continue;
      }

      if( strcmp(input->data[0],"BEG") == 0){
          //commande est beg
        args_len = 0;
        
        int valeur = atoi(input->data[1]);
      
        if (valeur == 0 && strcmp(input->data[1],"0") != 0){
         sendErreur("ERR premier argument pas un int",socket_w);
        freeValues(args, input);
        closeStream(socket_r, socket_w);
         continue;
       } else if (valeur==0){
         sendErreur("ERR le nombre de ressource ne peut pas etre egale a 0 ... (gros jugement)",socket_w);
        freeValues(args, input);
        closeStream(socket_r, socket_w);
         continue;
       } 
        else if (valeur<0){
         sendErreur("ERR le nombre de ressource ne peut pas etre inferieure a 0 ",socket_w);
        freeValues(args, input);
        closeStream(socket_r, socket_w);
         continue;
       } else{
         printf("SERVEUR A BIEN REÇU LE BEG \n");

         ressourcesLibres = calloc(valeur,sizeof(int));
         nbChaqueRess = calloc(valeur,sizeof(int));
         nbRessources = valeur;
         //printf("va send sendAck\n");
         sendAck(socket_w,-1);
         //printf("free args\n");
         //free(args);
         //printf("delete array\n");
       
         delete_array_string(input);
         closeStream(socket_r, socket_w);
         break;
       }

     }else{
      sendErreur("ERR mauvais commande attend BEG",socket_w);
        freeValues(args, input);
        closeStream(socket_r, socket_w);
     } 
    }
  }
  return;
}

void attendPro(socklen_t socket_len){
  int socket_fd;
  bool bonneCommande = false;
  printf("Server attend le pro \n");
  while(!bonneCommande){
  	 socket_fd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);

    while( socket_fd == -1) { 
      // attend le beg 
      socket_fd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
    }
    
    FILE *socket_r = fdopen (socket_fd, "r");
    FILE *socket_w = fdopen (socket_fd, "w");


    char *args = NULL; 
    size_t args_len=0;
    //int longueur = 0;
       //ssize_t cnt = getline (&args, &args_len, socket_r);
   // ssize_t cnt = getdelim (&args, &args_len,(int)' ', socket_r);
    printf("Right before getline \n");
    if(getline(&args,&args_len,socket_r) == -1){

      sendErreur("ERR mauvaise commande",socket_w);
      free(args);
 
      //delete_array_string(input);
        closeStream(socket_r, socket_w);
    }

    else{
      printf("Right before parseinputgetline \n");
      struct array_t_string *input = parseInputGetLine(args);
      imprimeArrayString(input);
             //parse (args," "); 
      printf("Right after parseinput \n");
      if(input->size != nbRessources + 1){
        sendErreur("ERR trop d'arguments",socket_w);
        freeValues(args, input);
        closeStream(socket_r, socket_w);
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

            sendErreur("ERR une argument n'est pas un int",socket_w);
                   freeValues(args, input);
        closeStream(socket_r, socket_w);
            continue;
          }

          else if (valeur==0){//une resssource aura un nb de ressource 
            sendErreur("ERR le nombre de ressource ne peut pas etre egale a 0 ... (gros jugement)",socket_w);
                    freeValues(args, input);
        closeStream(socket_r, socket_w);
              continue;
          }

          else{
             ressourcesLibres[longueur - 1] = valeur;
             nbChaqueRess[longueur - 1] = valeur;
             //free(args);
             longueur = longueur + 1;

             if (longueur == nbRessources){
               bonneCommande = true;
               sendAck(socket_w,-1);
               break;
             }
          }
        }
       }
      }
     else{
      sendErreur("ERR mauvais commande attend PRO",socket_w);
      free(args);
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
  //sigint est le (Signal Interrupt) Interactive attention signal.
  //sigint_handler est une fonction donc je donne un pointeur vers cette fonction
  //signal retourne la dernière valeur de la fonction ?

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

/*struct reponse {
  //char *args;
  struct array_t_string args;
  bool erreur;
  ssize_t cnt;
  int valeur;
}; */

//struct reponse

bool commEND (FILE *socket_r,FILE *socket_w){
      pthread_mutex_lock(&lockNbClient);
      pthread_mutex_lock(&lockCouDispa);
      if (nb_registered_clients==count_dispatched){
        pthread_mutex_lock(&locknbChaqRess);
        pthread_mutex_lock(&lockResLibres);
       
        for (int i = 0; i < nbRessources; ++i){

          if(nbChaqueRess[i] != ressourcesLibres[i]){
            sendErreur("ERR des ressources n'ont pas ete liberer",socket_w);
            //free(args);
            //fclose (socket_r);
            //fclose (socket_w);
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
        sendErreur("ERR il reste des clients \n",socket_w);
        return false;
      }
}

bool verifiePremierArgs (struct array_t_string *args,FILE *socket_r,FILE *socket_w){
      
        
        if (args->size < 2 ){
          sendErreur("ERR pas assez d'arguments",socket_w);
          return false;
        }        
        int valeur = atoi(args->data[1]);

        if ( valeur == 0 && strcmp(args->data[1],"0") != 0){
          sendErreur("ERR tid n'est pas un int",socket_w); 
          return false;
        }
        //si les arguments donc sont valide  
        return true;
}

void st_process_requests (server_thread * st, int socket_fd){
  // TODO: Remplacer le contenu de cette fonction
  FILE *socket_r = fdopen (socket_fd, "r");
  FILE *socket_w = fdopen (socket_fd, "w");

  while (true){
    
    char *args = NULL; size_t args_len = 0;
    printf("About to getline Client %d \n", socket_fd);
    if(getline(&args,&args_len,socket_r) == -1){//lit ce que le client envoie 
      //getline renvoie que il y a une erreur 
      sendErreur("ERR mauvaise commande",socket_w);

      if (args){
        free(args);
      }
      break;
    }
printf("Server a recu : %s du client %d \n",args,socket_fd);
    //printf("va parser l'input\n");
    struct array_t_string *input= parseInputGetLine(args);
    imprimeArrayString(input);
    
    //printf("commence les comparaisons\n");
    fflush(stdout);
    if(strcmp(input->data[0],"END") == 0){
      //free(cmd);
      //ma commande est end 
      commEND(socket_r,socket_w);
      delete_array_string(input);
      break;
    }


    else if(!verifiePremierArgs(input,socket_r,socket_w)){
        return; //j'ai eu une erreur et j'ai envoyer un message d'erreur 
    }

    if( strcmp(input->data[0],"INI") == 0){
        printf("rentre dans INI\n");
        //free(cmd);
        int *ressourcestemp = calloc(nbRessources, sizeof(int));

        if (!verifiePremierArgs(input,socket_r,socket_w)){
          free(ressourcestemp);
          delete_array_string(input);
          free(args);
          break;
        }

        int tidClient = atoi(input->data[1]);
        int longueur = 2;

        while(longueur != input->size){
          //printf("itère sur les arguments\n");
         
          int  valeur = atoi(input->data[longueur]);

          if ( valeur == 0 && strcmp(input->data[longueur],"0") != 0){
            sendErreur("ERR erreur une valeur n'est pas un int",socket_w);
            free(ressourcestemp);
            free(args);
            delete_array_string(input);
            break;
          }

          else if (valeur<0){
            sendErreur("ERR erreur une valeur est negative",socket_w);
            free(ressourcestemp);
            free(args);
            delete_array_string(input);
            break;
          }

          else{
          //  printf("pas d'erreur sur l'argument\n");
           ressourcestemp[longueur-2] = valeur;
           longueur = longueur + 1;
         }           
       }

       if (longueur-2 != nbRessources){//les ressoruces n'ont pas tous été déclaré
         
          //printf("longueur = %d\n",longueur);
         // printf("nbRessources = %d\n",longueur);
          sendErreur("ERR mauvais nombre de ressources specifier",socket_w);
          free(ressourcestemp);
          delete_array_string(input);
          free(args);
          break;
       }

          //struct Client *maxTemp;
         /* maxTemp = calloc(nb_registered_clients + 1, sizeof(struct Client));

          pthread_mutex_lock(&lockMax);

          for (int i = 0; i < nb_registered_clients; ++i){
            maxTemp[i] = max[i];
          }*/

      // printf("pas de probleme avec les arguments\n");
       struct Client nouvClient = {tidClient,ressourcestemp};
        //maxTemp[nb_registered_clients + 1] = nouvClient; 
       //printf("va prendre le lock du max\n");
       pthread_mutex_lock(&lockMax);
       //printf("va push dans le max le nouveau client\n");
       int retour1 = push_back(&max,&nouvClient);//met le client dans l'array max
       
       if (retour1 == -1){
        sendErreur("ERR erreur interne",socket_w);
        pthread_mutex_unlock(&lockMax);
          //pthread_mutex_unlock(&lockNbClient);
        free(args);
        delete_array_string(input);
        free(ressourcestemp);
       // pthread_mutex_unlock(&lockMax);
        break;
       }
       printf("unlock le max\n");
       pthread_mutex_unlock(&lockMax);
          //struct array_t besoinTemp = new_array(max.capacity);

       int *besoinTemp = calloc(nbRessources,sizeof(int));
       //printf("va initialiser le tableau des besoins\n");
       for (int i = 0; i < nbRessources; ++i){
         besoinTemp[i] = ressourcestemp[i]; //initialise le tableau des besoins 
       }                                     //le client a besoin du maximum
          //free(max);
         // max = maxTemp;
       //printf("va lock le talbeau des besoins\n");
       struct Client nouvClientBesoin = {tidClient,besoinTemp};
       pthread_mutex_lock(&lockBesoin);
       retour1 = push_back(&besoin,&nouvClientBesoin);
       if (retour1 == -1){
         sendErreur("ERR erreur interne",socket_w);
         //printf("va unlock le tableau des besoins\n");
         pthread_mutex_unlock(&lockBesoin);
            //pthread_mutex_unlock(&lockNbClient);
         //printf("va liberer ressouces temp\n");
         free(ressourcestemp);
         break;
       }
       pthread_mutex_unlock(&lockBesoin);
       //printf("met a jour le nombre de client\n");
       pthread_mutex_lock(&lockNbClient);
       nb_registered_clients = nb_registered_clients + 1;  //j'ai un client de plus 
       pthread_mutex_unlock(&lockNbClient);
       //printf("envoie un ack\n");
       sendAck(socket_w,tidClient);
       break;
    }


    else if (strcmp(input->data[0],"REQ") == 0){
      //free(cmd);
      int *ressourcesDem = calloc(nbRessources, sizeof(int));
      
      int tidClient = atoi(input->data[1]);
      //int tidClient = valeur;
      int longueur = 2;

      while(longueur + 2 != input->size){
   
        /*struct reponse retour;
        retour= verifiePremierArgs(args,args_len,socket_r,socket_w);*/
        int  valeur = atoi(input->data[longueur]);

        if ( valeur == 0 && strcmp(args,"0") != 0){
          sendErreur("ERR erreur une valeur n'est pas un int",socket_w);
          free(ressourcesDem);
          delete_array_string(input);
          free(args);
            break;
        }

        else{
          ressourcesDem[longueur] = valeur;
          longueur = longueur + 1;
        }       
      }

      if (longueur-2 != nbRessources){//les ressoruces n'ont pas tous été déclaré
         sendErreur("ERR mauvais nombre de ressources specifier",socket_w);
          free(ressourcesDem);
          delete_array_string(input);
          free(args);
          break;
       }
      //printf("va unlock le tableau des besoins\n");
      pthread_mutex_lock(&lockBesoin);

      int j = 0;

      //while(j!= nbRessources){
        while(j!= besoin.size){
        if ((*besoin.data)[j].tid == tidClient){
          break;
        }

        j = j + 1;
      }

      if (j==nbRessources){
        sendErreur("ERR le client n'a pas ete initiliase",socket_w);   
        //printf("va unlock le tableau des besoins\n");
        pthread_mutex_unlock(&lockBesoin);
        free(ressourcesDem);
        break;
      }

      int *ressBesoinClient = (*besoin.data)[j].ressClient;

      pthread_mutex_lock(&lockResLibres);

      //bool *assezRessPourMax = calloc(nbRessources, sizeof(bool));
      int assezRessPourMax = 0;
      for (int i = 0; i < nbRessources; ++i){
        //je regarde si j'ai assez de ressources de chaque pour allouer au client 
        //et si j'ai une erreur puisque demande plus qu'il en a besoin 
        if(ressourcesDem[i] > ressourcesLibres[i]){
          sendWait(max_wait_time,socket_w,*max.data[j]);
          free(ressourcesDem);
          pthread_mutex_unlock(&lockResLibres); 
          printf("va unlock le tableau des besoins\n");
          pthread_mutex_unlock(&lockBesoin);
          delete_array_string(input);
          free(args);
          fclose (socket_r);
          fclose (socket_w);
          return;
        }

        else if(ressourcesDem[i]>ressBesoinClient[i]){
          sendErreur("ERR client demande plus de ressources que le max declarer dans ini",socket_w);
          
          pthread_mutex_unlock(&lockResLibres); 
          printf("va unlock le tableau des besoins\n");
          pthread_mutex_unlock(&lockBesoin);
          free(ressourcesDem);
                  freeValues(args, input);
        closeStream(socket_r, socket_w);
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
      

      free(ressourcesDem);

      pthread_mutex_unlock(&lockAllouer);
      pthread_mutex_unlock(&lockResLibres); 
      printf("va unlock le tableau des besoins\n");
      pthread_mutex_unlock(&lockBesoin);
      free(ressourcesDem);
              freeValues(args, input);
      break;
    }

    else if(strcmp(input->data[0],"CLO") == 0){

      pthread_mutex_lock(&lockClientEnd);
      clients_ended += 1;
      pthread_mutex_unlock(&lockClientEnd);      

      //int tidClient = valeur;
     /* struct reponse retour;
      retour= verifiePremierArgs(args,args_len,socket_r,socket_w);x

      if (retour.erreur == true){
          break;
        }*/

      int tidClient = atoi(input->data[1]);
     // free(cmd);
      pthread_mutex_lock(&lockAllouer);
      int positionAllouer = 0;

      while(positionAllouer!= (allouer.size - 1) ){//je cherche la position du client concerné dans l'array
        if ((*allouer.data)[positionAllouer].tid == tidClient){
          break;
        }

        positionAllouer = positionAllouer + 1;
      }

      if (positionAllouer == nbRessources){
        sendErreur("ERR le client n'a pas ete initiliase",socket_w);   
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
      //printf("va unlock le tableau des besoins\n");
      pthread_mutex_lock(&lockBesoin);

      int positionBesoin = 0;

    /*  while(positionBesoin!= (besoin.size - 1) ){//je cherche aussi le client
        if (besoin.data[positionBesoin]->tid == tidClient){
          break;
        }

        positionBesoin = positionBesoin + 1;
      }

      if (positionAllouer == nbRessources){
        sendErreur("ERR le client n'a pas ete initiliase",socket_w);   
        pthread_mutex_unlock(&besoin);
        break;
      }*/

      besoin = deleteClientInArray(&besoin,tidClient);

      pthread_mutex_unlock(&lockBesoin);

      pthread_mutex_lock(&lockMax);

      max = deleteClientInArray(&max,tidClient);

      pthread_mutex_unlock(&lockMax);

      pthread_mutex_lock(&lockCouDispa);
      //count_dispatched
      count_dispatched += 1;

      pthread_mutex_unlock(&lockCouDispa);


      sendAck(socket_w,tidClient);
    }

    else{
      //free(cmd);
      sendErreur("ERR commande inconnu",socket_w); 
      delete_array_string(input);
      free(args);
      break;  
    }        
   //}

    /*if (!args || cnt < 1 || args[cnt - 1] != '\n')//le buffer args est vide ou j'ai moins de 1 caractère qui a été écrit et ou mon dernier caractère n'est pas égale à une fin de ligne
    {
      printf ("Thread %d received incomplete cmd=%s!\n", st->id, cmd);
      break;
    }*/

   // printf ("Thread %d received the command: %s%s", st->id, cmd, args);

    //fprintf (socket_w, "ERR Unknown command\n");
   // free (args);
}

fclose (socket_r);
fclose (socket_w);
  // TODO end
}



void *st_code (void *param){
  server_thread *st = (server_thread *) param;

  int thread_socket_fd = -1;
  //printf("rentre dans stcode\n");
  // Boucle de traitement des requêtes.
  while (accepting_connections)
  {
    // Wait for a I/O socket.
    printf("Server commence le wait\n");
    thread_socket_fd = st_wait();
    printf("Server a accepté le client %d \n", thread_socket_fd);
    if (thread_socket_fd < 0)
    {
      fprintf (stderr, "Time out on thread %d.\n", st->id);//reesaye plus tard
      continue;
    }

    if (thread_socket_fd > 0)//si j'ai eu une requete
    {
      //printf("va rentrer dans st_process_requests\n");
      //printf("\n");
      printf("Server va process la requête de %d \n", thread_socket_fd);
      st_process_requests (st, thread_socket_fd);
      //printf("est sorti de st_process_requests\n");
      //printf("\n");
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
for (i=0;formatValide && assezRessources && i<num_resources;i++){
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
for (int i=0; i<num_resources;i++){
    ressourcesLibres[i] -= client_request[i];
    arrayAllouesClient[client_id][i];
    arrayBesoinsClient[client_id][i] -= client_request[i];
}


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

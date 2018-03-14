#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */

#include "server_thread.h"

#include <netinet/in.h>
#include <netdb.h>

#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include<pthread.h>
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

pthread_mutex_t lock;

unsigned int server_socket_fd;

// Nombre de client enregistré.
int nb_registered_clients;

int *ressourcesLibres;
int **max;
int **allouer;
int **besoin;


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

static void sigint_handler(int signum) {
  // Code terminaison.
  accepting_connections = 0;//je n'acccepte plus de connection
}

void erreur(const char *message){
    perror(message);

}

void sendErreur(const char *message){
  //envoie au client le message d'erreur 
}


char** append(char** tableau, char* element, int ancienIndex){

int longueur = 0;
char** copie;
  if (tableau[ancienIndex]==0)
  {
    
    copie = (char **)malloc((2*ancienIndex  )*sizeof(char*));
    while(longueur != ancienIndex){
          copie[longueur] = strcpy(copie[longueur],tableau[longueur]);
        //free tableau[longueur]?
          longueur = longeur + 1;
    }
    copie[longueur + 1] = element;
    copie[2*ancienIndex] = 0; 
  }

  if (tableau[ancienIndex]!= 0 && tableau[ancienIndex + 1] != 0)
  {
    
  }

}


void st_init ()
{
  // Handle interrupt
  signal(SIGINT, &sigint_handler); 
  //sigint est le (Signal Interrupt) Interactive attention signal.
  //sigint_handler est une fonction donc je donne un pointeur vers cette fonction
  //signal retourne la dernière valeur de la fonction ?

  // Initialise le nombre de clients connecté.
  nb_registered_clients = 0;

  int retour_init;
  retour_init = pthread_mutex_init(lock,NULL);//initialise le mutex
  if (retour_init != 0)
  {
    erreur("erreur init mutex");
  }

  struct sockaddr_in thread_addr;

  socklen_t socket_len = sizeof (thread_addr);

  int socketFd;

  socketFd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
  
   while( socketFd == -1) { 
      // attend le beg 
      socketFd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
   }

   char *args = NULL; 
   size_t args_len=0;
  
   FILE *socket_r = fdopen (socket_fd, "r");
  
   ssize_t cnt = getline (&args, &args_len, socket_r);
   
   parse (args," "); 

   int nbRessources;

   if(nbmots > 2  || nbmots <2){
      sendErreur("mauvais nombre arguments")
    }
   
   if(premiermot == "BEG"){

      if (deuxiememot est un int )
      {
         ressourcesLibres = calloc(deuxiememot,sizeof(int));
         nbRessources = deuxiememot;
      }

      else{
        sendErreur("premier argument pas un int");
      }
   }

   else{
    sendErreur("mauvais commande attend BEG");
   }

   
   socketFd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
  
   while( socketFd == -1) { 
      // attend le pro 
      socketFd = accept(server_socket_fd,(struct sockaddr *)&thread_addr, &socket_len);
   }

  arraydemots = parse (args," "); 

  int longeur=0;
  while(pas fin de arraydemots){
    longeur++;
  }

   if(longeur- 1 > nbRessources  || longeur - 1 < nbRessources){
      sendErreur("mauvais nombre d'arguments");
    }
   
   if(premiermot == "PRO"){
    for (int i = 1; i < longueur; ++i)
    {
        if (arraydemots[i] est un int )
        {
           ressourcesLibres[i-1] = arraydemots[i];
        }

        else{
          sendErreur("un argument est pas un int");
        }
      }
   }

   else{
    sendErreur("mauvais commande attend PRO");
   }




   /*char *args = NULL; size_t args_len=0;

   FILE *socket_r = fdopen (socket_fd, "r");
   ssize_t cnt = getline (&args, &args_len, socket_r);
   args  = strtok(args,"\n");
   char** mots;


   args = strtok(args," ");*/





  // TODO

  // Attend la connection d'un client et initialise les structures pour
  // l'algorithme du banquier.

  // END TODO
}

void
st_process_requests (server_thread * st, int socket_fd)
{
  // TODO: Remplacer le contenu de cette fonction
  FILE *socket_r = fdopen (socket_fd, "r");
  FILE *socket_w = fdopen (socket_fd, "w");

  while (true)
  {
    char cmd[4] = {NUL, NUL, NUL, NUL};
    if (!fread (cmd, 3, 1, socket_r))
      break;
    char *args = NULL; size_t args_len = 0;
    ssize_t cnt = getline (&args, &args_len, socket_r);
    if (!args || cnt < 1 || args[cnt - 1] != '\n')//le buffer args est vide ou j'ai moins de 1 caractère qui a été écrit et ou mon dernier caractère n'est pas égale à une fin de ligne
    {
      printf ("Thread %d received incomplete cmd=%s!\n", st->id, cmd);
      break;
    }

    printf ("Thread %d received the command: %s%s", st->id, cmd, args);

    fprintf (socket_w, "ERR Unknown command\n");
    free (args);
  }

  fclose (socket_r);
  fclose (socket_w);
  // TODO end
}


/*void
st_signal ()
{
  // TODO: Remplacer le contenu de cette fonction



  // TODO end
}*/

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

void *st_code (void *param)
{
  server_thread *st = (server_thread *) param;

  int thread_socket_fd = -1;

  // Boucle de traitement des requêtes.
  while (accepting_connections)
  {
    // Wait for a I/O socket.
    thread_socket_fd = st_wait();
    if (thread_socket_fd < 0)
    {
      fprintf (stderr, "Time out on thread %d.\n", st->id);//reesaye plus tard
      continue;
    }

    if (thread_socket_fd > 0)//si j'ai eu une requete
    {
      st_process_requests (st, thread_socket_fd);
      close (thread_socket_fd);
    }
  }
  return NULL;
}


//
// Ouvre un socket pour le serveur.
//
void
st_open_socket (int port_number)
{
  server_socket_fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket_fd < 0)
    perror ("ERROR opening socket");

  if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &(int){ 1 }, sizeof(int)) < 0) {
    perror("setsockopt()");
    exit(1);
  }

  struct sockaddr_in serv_addr;
  memset (&serv_addr, 0, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons (port_number);

  if (bind
      (server_socket_fd, (struct sockaddr *) &serv_addr,
       sizeof (serv_addr)) < 0)
    perror ("ERROR on binding");

  listen (server_socket_fd, server_backlog_size);
}


//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
  if (fd == NULL) fd = stdout;
  if (verbose)
  {
    fprintf (fd, "\n---- Résultat du serveur ----\n");
    fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
    fprintf (fd, "Requêtes : %d\n", count_wait);
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

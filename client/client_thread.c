/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client_thread.h"

// Socket library
//#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <strings.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int port_number = -1;
int num_request_per_client = -1;
int num_resources = -1;
int *provisioned_resources = NULL;

// Variable d'initialisation des threads clients.
unsigned int count = 0;
void flushmoica();

// Variable du journal.
// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nombre de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nombre de client qui se sont terminés correctement (ACC reçu en réponse à END)
unsigned int count_dispatched = 0;

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;
//https://www.geeksforgeeks.org/socket-programming-cc/
//Basé sur https://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner

// Vous devez modifier cette fonction pour faire l'envoie des requêtes
// Les ressources demandées par la requête doivent être choisies aléatoirement
// (sans dépasser le maximum pour le client). Elles peuvent être positives
// ou négatives.
// Assurez-vous que la dernière requête d'un client libère toute les ressources
// qu'il a jusqu'alors accumulées.

//Génère nombre aléatoire entre 0 et max
//https://www.tutorialspoint.com/c_standard_library/c_function_rand.htm
//Voir https://wiki.sei.cmu.edu/confluence/display/c/MSC30-C.+Do+not+use+the+rand%28%29+function+for+generating+pseudorandom+numbers
int make_random(int max_resources){
    return rand() % (max_resources+1);
}

void
send_request (int client_id, int request_id, int socket_fd,char* message) {
    FILE *socket_w = fdopen(socket_fd, "w");
    fprintf(socket_w, "%s", message);
    fflush(socket_w);
    printf("Client sent %s", message);
    
    FILE *socket_r = fdopen(socket_fd, "r");
    char *args = NULL;
    size_t args_len = 0;
    //TODO: Changer ceci, cause seg fault
    ssize_t cnt = getdelim(&args, &args_len, (int) ' ', socket_r);
    switch (cnt) {
        case -1:
            perror("Erreur réception client");
            break;
        default:
            break;
    }
    printf("Ce que client a reçu %s", args);
    /*
    char toReceive[20];
    memset (toReceive,'0', 20);
    int res = recv(socket_fd,toReceive, strlen(toReceive),0);
    switch (res){
        case -1:
            perror ("Erreur réception client");
            break;
        case 0:
            perror ("Remote closed connexion");
            break;
        default:
            break;
    }
    printf("%s \n", toReceive);
    */
    // TP2 TODO:END
}

//Basé sur https://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner
int client_connect_server()
{
    printf("Un client essaie de créer un socket \n");
    int client_socket_fd=-1;

    //Crée un socket via addresse IPV4 et TCP ou UDP
    if ((client_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("ERROR opening socket");
        return 1;
    }

    struct hostent *hostInternet;
    if ((hostInternet = gethostbyname("localhost")) == NULL){
        perror("ERROR finding IP");
        return client_socket_fd;
    };
    //Addresse serveur
    struct sockaddr_in server_address;
    memset (&server_address, 0, sizeof (server_address));
    server_address.sin_family = AF_INET;
   
    server_address.sin_port = htons(port_number);
    server_address.sin_addr.s_addr = INADDR_ANY;
    
    //Padding nécessaire posix
    memset (server_address.sin_zero, 0, sizeof (server_address.sin_zero));
    
    printf("** Client désire se connecter ** \n");
    flushmoica();

    if (connect(client_socket_fd,(struct sockaddr *) &server_address, sizeof(server_address)) < 0 ){
        perror("ERROR connexion");
        return client_socket_fd;
    };
    printf("+_+ Client est connecté au serveur +_+ \n");
    flushmoica();

    return client_socket_fd;
}

void flushmoica(){
    fflush(stdout);
}

void *
ct_code (void *param)
{
    int client_socket_fd = client_connect_server();
    //Client connecté au serveur

  client_thread *ct = (client_thread *) param;

    //Initialise le client
    char message[50]="INI";
    char append[5];
    sprintf(append,"%d",ct->id); // put the int into a string
    strcat(message, append);

    memset(append, 0, sizeof append);
    //Choisit valeurs max de façon random
    for (int i =0; i < num_resources;i++){
        //TODO: Vérifier si ce code segfault
        //TODO: Fetch le vrai nb max
        //snprintf(message, sizeof message, "%d", make_random(10));
        sprintf(append,"%d",make_random(10)); // put the int into a string
        strcat(message, append); // modified to append string
    }
    //Envoie la requête INI
    send_request(ct->id,-1,client_socket_fd,message);

    printf("Waiting on server response (expecting an ACK");

  for (unsigned int request_id = 0; request_id < num_request_per_client;
      request_id++)
  {

      // TP2 TODO
      // Vous devez ici coder, conjointement avec le corps de send request,
      // le protocole d'envoi de requête.

      send_request (ct->id, request_id, client_socket_fd,message);

      // TP2 TODO:END

      /* Attendre un petit peu (0s-0.1s) pour simuler le calcul.  */
      usleep (random () % (100 * 1000));
      /* struct timespec delay;
       * delay.tv_nsec = random () % (100 * 1000000);
       * delay.tv_sec = 0;
       * nanosleep (&delay, NULL); */

  }
    count_dispatched++;

    //https://stackoverflow.com/questions/4160347/close-vs-shutdown-socket
  shutdown(client_socket_fd, SHUT_RDWR);
  close(client_socket_fd);
  return NULL;
}


//
// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
//
void
ct_wait_server ()
{

  // TP2 TODO: IMPORTANT code non valide.

  sleep (4);

  // TP2 TODO:END

}


void
ct_init (client_thread * ct)
{
  ct->id = count++;
}

void
ct_create_and_start (client_thread * ct)
{
  pthread_attr_init (&(ct->pt_attr));
  pthread_create (&(ct->pt_tid), &(ct->pt_attr), &ct_code, ct);
  pthread_detach (ct->pt_tid);
}

//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
  if (fd == NULL)
    fd = stdout;
  if (verbose)
  {
    fprintf (fd, "\n---- Résultat du client ----\n");
    fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
    fprintf (fd, "Requêtes : %d\n", count_on_wait);
    fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
    fprintf (fd, "Clients : %d\n", count_dispatched);
    fprintf (fd, "Requêtes envoyées: %d\n", request_sent);
  }
  else
  {
    fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
        count_invalid, count_dispatched, request_sent);
  }
}

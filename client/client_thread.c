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

int port_number = -1;
int num_request_per_client = -1;
int num_resources = -1;
int *provisioned_resources = NULL;

// Variable d'initialisation des threads clients.
unsigned int count = 0;


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
send_request (int client_id, int request_id, int socket_fd)
{
  // TP2 TODO
    //char* text = NULL;
    //text = (char *) malloc(64);
    //FILE *socket_r = fdopen (socket_fd, "r");
    //FILE *socket_w = fdopen (socket_fd, "w");
    //fwrite("ACK TEST",strlen("ACK TEST")+1,1,socket_w);
  static char *toSend = "INI 100 50 25 10";
  int ret;
    //int send(int sockfd, const void *msg, int len, int flags);
  if ((ret = send(socket_fd,toSend, strlen(toSend),0))==-1)
       perror("write() error");
    if (ret<strlen(toSend)){
        perror ("Il a pas réussi à tout send");
    }
  //fprintf (stdout, "Client %d is sending its %d request\n", client_id,
    //mak  request_id);

    //int serverresponse =
    printf("Client reading : ");
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
  // TP2 TODO:END

}

//allo
//Basé sur https://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner
int client_connect_server()
{
    int client_socket_fd=-1;

    //Crée un socket via addresse IPV4 et TCP ou UDP
    if ((client_socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0){
        perror("ERROR opening socket");
        return 1;
    }
    //Ici on doit aller chercher l'IP du serveur d'une manière ou d'une autre
    //http://www.gnu.org/software/libc/manual/html_node/Host-Names.html
    //http://man7.org/linux/man-pages/man3/inet_pton.3.html
    //https://linux.die.net/man/3/inet_aton
    //http://www.qnx.com/developers/docs/6.5.0/index.jsp?topic=%2Fcom.qnx.doc.neutrino_lib_ref%2Fh%2Fhostent.html
    struct hostent *hostInternet;
    if ((hostInternet = gethostbyname("localhost")) == NULL){
        //TODO: Peut-être devra créer un hostent si besoin de réutiliser
        //http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
        perror("ERROR finding IP");
        return client_socket_fd;
    };
    //Addresse serveur
    struct sockaddr_in server_address;
    //Met des 0 partout en memoire : https://www.tutorialspoint.com/c_standard_library/c_function_memset.htm
    memset (&server_address, '0', sizeof (server_address));
    server_address.sin_family = AF_INET;
    //server_address.sin_addr.s_addr = INADDR_ANY;
    /*htons()host to network short
htonl()host to network long
ntohs()network to host short
ntohl()network to host long
     */
    server_address.sin_port = htons(port_number);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    //Copie la valeur de l'adresse serveur dans le sockaddr_in
    //bcopy(hostInternet->h_addr, &server_address.sin_addr.s_addr,hostInternet->h_length);
    memset (server_address.sin_zero, '\0', sizeof (server_address.sin_zero));
    //int yes=1;
    // lose the pesky "Address already in use" error message
    /*if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
        perror("setsockopt");
        exit(1);
    }*/
    if (connect(client_socket_fd,(struct sockaddr *) &server_address, sizeof(server_address)) < 0 ){
        perror("ERROR connexion");
        return client_socket_fd;
    };

    return client_socket_fd;
}

void *
ct_code (void *param)
{
  //int socket_fd = -1;
  client_thread *ct = (client_thread *) param;

    int client_socket_fd = client_connect_server();
    //Client connecté au serveur

  for (unsigned int request_id = 0; request_id < num_request_per_client;
      request_id++)
  {

      // TP2 TODO
      // Vous devez ici coder, conjointement avec le corps de send request,
      // le protocole d'envoi de requête.

      send_request (ct->id, request_id, client_socket_fd);

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

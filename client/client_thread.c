/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>
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
int num_clients = 0;
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
unsigned int count_dispatched = 0;                      //ACK reçu en réponse à CLO plutot?

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;
 

pthread_mutex_t lockCount_acc;
pthread_mutex_t lockCount_wait;
pthread_mutex_t lockCount_inv;
pthread_mutex_t lockCount_disp;
pthread_mutex_t lockReqSent;
//pthread_mutex_t lockFinClient;

void ct_start(){
	int retour;
	retour = pthread_mutex_init(&lockCount_acc,NULL);
	if (retour != 0){
		perror("Erreur init mutex pour nombre de ack (nombre de commande executer)");
	}

	retour = pthread_mutex_init(&lockCount_wait,NULL);
	if (retour != 0){
		perror("Erreur init mutex pour nombre de wait ");
	}

	retour = pthread_mutex_init(&lockCount_inv,NULL);
	if (retour != 0){
		perror("Erreur init mutex pour nombre de requete invalide");
	}

	retour = pthread_mutex_init(&lockReqSent,NULL);
	if (retour != 0){
		perror("Erreur init mutex pour nombre requêtes envoyées");
	}

	/*finClient = calloc(num_clients,sizeof(bool));

	retour = pthread_mutex_init(&lockFinClient,NULL);
	if (retour != 0){
		perror("Erreur init mutex pour nombre de client terminé");
	}*/

	retour = pthread_mutex_init(&lockCount_disp,NULL);
	if (retour != 0){
		perror("Erreur init mutex pour nombre de client terminé");
	}

}


int make_random(int max_resources){
    return rand() % (max_resources+1);// fait + 1 pour povoir demander le maximum d'une ressource
}

int make_random_req(int max_resources){
	int nombre = make_random(max_resources);
	if (((double)rand() / (double)RAND_MAX) < 0.5)//code pris de https://stackoverflow.com/questions/6218399/how-to-generate-a-random-number-between-0-and-1
	{
		return nombre * -1;
	}

	return nombre;
}

void flushmoica(){
    fflush(stdout);
}


// Vous devez modifier cette fonction pour faire l'envoie des requêtes
// Les ressources demandées par la requête doivent être choisies aléatoirement
// (sans dépasser le maximum pour le client). Elles peuvent être positives
// ou négatives.
// Assurez-vous que la dernière requête d'un client libère toute les ressources
// qu'il a jusqu'alors accumulées.
int 
send_request (int client_id, int request_id, int socket_fd,char* message){
    printf("Client %d attempting to send %s \n",client_id,message);
    //FILE *socket_w = fdopen(socket_fd, "w");
    //fprintf(socket_w, "%s", message);
    //fflush(socket_w);
    //printf("Message sent = %s \n",message);
    if (send (socket_fd, message, strlen(message),0)<0){
        printf("Send failed");
        return -1;
    }
    
    //FILE *socket_r = fdopen(socket_fd, "r");
    //char* args; // = (char*) malloc (10*sizeof(char));
	/*if (args==NULL){
		perror("Buffer marche po");
		exit(1);
	}*/

    //size_t args_len = 0;
    
    printf("Client %d waiting for response..\n", client_id);
     
    //ssize_t cnt = getline(&args, &args_len, socket_r);//peut mettre dans un while tant que cnt = -1
    if (recv (socket_fd, message, 50,0)<0){
        printf("Receive failed");
        return -1;
    }
    printf("Client %d received %s \n", client_id, message);
    /*
    switch (cnt) {
        case -1:
            perror("Erreur réception client \n");
            return 0;
            break;
        default:

            break;
    }
   */

    // TP2 TODO:END
    //printf("Client %d close le stream \n", socket_fd);
    //flushmoica();
    //fclose(socket_w);
    //fclose(socket_r);
    if (strcmp(message,"ACK")){
        pthread_mutex_lock(&lockCount_acc);
        count_accepted += 1;
        pthread_mutex_unlock(&lockCount_acc);
        return 1;
    }else{
    	if (strstr(message,"ERR")!=NULL){
    		pthread_mutex_lock(&lockCount_inv);
        	count_invalid += 1;
        	pthread_mutex_unlock(&lockCount_inv);
    	}

    	else if(strstr(message,"WAIT")){
            pthread_mutex_lock(&lockCount_wait);
        	count_on_wait += 1;
        	pthread_mutex_unlock(&lockCount_wait);
    	}

    	return 0;
    }
    //return 0;
}

//Basé sur https://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner
int client_connect_server(){
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
    
    printf("** Client FD %d désire se connecter ** \n", client_socket_fd);
    //flushmoica();

    if (connect(client_socket_fd,(struct sockaddr *) &server_address, sizeof(server_address)) < 0 ){
        perror("ERROR connexion");
        return client_socket_fd;
    };
    printf("+_+ Client FD %d est connecté à un serveur +_+ \n", client_socket_fd);
    flushmoica();

    return client_socket_fd;
}



void * ct_code (void *param){
    int client_socket_fd = client_connect_server();
    client_thread *ct = (client_thread *) param;

    //Initialise le client
    char message[50]="INI";
    char append[5];
    sprintf(append," %d",ct->id); // put the int into a string
    strcat(message, append);

    memset(append, 0, sizeof append);
    //Choisit valeurs max de façon random
    //printf("nombre de ressource = %d\n",num_resources);
    for (int i =0; i < num_resources;i++){
        //TODO: Vérifier si ce code segfault
        //snprintf(message, sizeof message, "%d", make_random(10));
        sprintf(append," %d",make_random(provisioned_resources[i])); // put the int into a string
        strcat(message, append); // modified to append string
    }
    //Envoie la requête INI
    int retour= send_request(ct->id,-1,client_socket_fd,message);
    if (retour == 0){
    	printf("Client %d FD %d - ACK NOT RECEIVED \n", ct->id, client_socket_fd);
    }

    else{
    	printf("Client %d FD %d - ACK RECEIVED \n", ct->id, client_socket_fd);
    }

  //  shutdown(client_socket_fd, SHUT_RDWR);
    printf("Closing socket %d for Client %d \n", client_socket_fd, ct->id);
    close(client_socket_fd);
  for (unsigned int request_id = 0; request_id < num_request_per_client;
      request_id++){
        printf("Client %d attempting to REQ \n", ct->id);
  		client_socket_fd = client_connect_server() ;
        printf("New client socket fd %d \n", client_socket_fd);
  		char message1[50]  = "REQ";
  		char append1[5]; 
  		sprintf(append1," %d",ct->id);
  		strcat(message1,append1);

  		for (int i = 0; i < count; ++i){
  			sprintf(append," %d",make_random_req(provisioned_resources[i])); 
        	strcat(message, append);
  		}
  		retour = send_request (ct->id, request_id, client_socket_fd,message);
  		//int retour= send_request(ct->id,-1,client_socket_fd,message);
    	
    	if (retour == 0){
	    	    	printf("Client %d - ACK NOT RECEIVED \n", client_socket_fd);
   		 }

    	else{
	    		printf("Client %d - ACK RECEIVED \n", client_socket_fd);
    	}
    	close(client_socket_fd);

	    //printf("reçu %d\n",retour);
	    //fflush(stdout);

      // TP2 TODO
      // Vous devez ici coder, conjointement avec le corps de send request,
      // le protocole d'envoi de requête.


  	//TODO: analyser les erreurs retourné s'il y en a et faire clo a la fin des requetes 	
      

      // TP2 TODO:END

      /* Attendre un petit peu (0s-0.1s) pour simuler le calcul.  */
      usleep (random () % (100 * 1000));
      /* struct timespec delay;
       * delay.tv_nsec = random () % (100 * 1000000);
       * delay.tv_sec = 0;
       * nanosleep (&delay, NULL); */

  }

  client_socket_fd = client_connect_server();
  char message1[50]  = "CLO";
  char append1[5]; 
  sprintf(append1," %d",ct->id);
  strcat(message1,append1);
  retour = send_request (ct->id, num_request_per_client, client_socket_fd,message);

  if(retour == 0){
		printf("pas reçu ACK\n");
	    fflush(stdout);
   }

  else{
    	printf("reçu ACK\n");
   		fflush(stdout);
   		pthread_mutex_lock(&lockCount_disp);
   		count_dispatched++;
   		//pthread_mutex_lock(&lockFinClient);

  		//finClient[count_dispatched-1]= true;
  		//pthread_mutex_unlock(&lockFinClient);
   		pthread_mutex_unlock(&lockCount_disp);
   }

  close(client_socket_fd);

  
   // count_dispatched++;

    //https://stackoverflow.com/questions/4160347/close-vs-shutdown-socket
 /* shutdown(client_socket_fd, SHUT_RDWR);
  close(client_socket_fd);*/

  return NULL;
}


//
// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
//
void ct_wait_server (){

  // TP2 TODO: IMPORTANT code non valide.

  //sleep (4);
  while(count_dispatched != num_clients);

  // TP2 TODO:END

}


void ct_init (client_thread * ct)
{
  ct->id = count++;
}

void ct_create_and_start (client_thread * ct){
  pthread_attr_init (&(ct->pt_attr));
  pthread_create (&(ct->pt_tid), &(ct->pt_attr), &ct_code, ct);
  pthread_detach (ct->pt_tid);
}

//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void st_print_results (FILE * fd, bool verbose){
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

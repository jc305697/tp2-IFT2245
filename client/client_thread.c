/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 700
#include "../array/dyn_array.c"
#include "client_thread.h"
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
struct array_t_string *parseInput(char *input);

// Variable d'initialisation des threads clients.
unsigned int count = 0;

// Variable du journal.
// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nombre de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nombre de client qui se sont terminés correctement 
// (ACC reçu en réponse à CLO)
unsigned int count_dispatched = 0;                      

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;

pthread_mutex_t lockCount_acc;
pthread_mutex_t lockCount_wait;
pthread_mutex_t lockCount_inv;
pthread_mutex_t lockCount_disp;
pthread_mutex_t lockReqSent;
pthread_mutex_t lockCount;


/* Réduit duplication de code */
void lockIncrementUnlock(pthread_mutex_t mut, unsigned int *compteur){
  pthread_mutex_lock(&mut);
  *compteur += 1;
  pthread_mutex_unlock(&mut); 
}

/*
    Initialise les mutex utilisés dans ce programme
*/
void ct_start(){
	if (pthread_mutex_init(&lockCount_acc,NULL) != 0)
		perror("Erreur init mutex pour nombre de ack \
                (nombre de commande executer)");
	if (pthread_mutex_init(&lockCount_wait,NULL) != 0)
		perror("Erreur init mutex pour nombre de wait ");
	if (pthread_mutex_init(&lockCount_inv,NULL) != 0)
		perror("Erreur init mutex pour nombre de requete invalide");
	if (pthread_mutex_init(&lockReqSent,NULL) != 0)
		perror("Erreur init mutex pour nombre requêtes envoyées");
	if (pthread_mutex_init(&lockCount_disp,NULL) != 0)
		perror("Erreur init mutex pour nombre de client terminé");
  if (pthread_mutex_init(&lockCount,NULL) != 0)
    perror("Erreur init mutex pour le count des threads");
}


int make_random(int max_resources){
    //+1 permet de demander le maximum d'une ressource
    return rand() % (max_resources+1);
}


/*
  Obtenir un nombre négatif ou positif au hasard
  Code pris de 
  https://stackoverflow.com/questions/6218399/
  how-to-generate-a-random-number-between-0-and-1
*/
int make_random_req(int max_resources){
    
	int nombre = make_random(max_resources);
	if (((double)rand() / (double)RAND_MAX) < 0.5)
	{
        
		return nombre * -1;
	}else{
	    return nombre;
    }
}





/* 
    Envoie les messages reçus en paramètre au serveur 
    et attend + traite la réponse 
*/
int 
send_request (int client_id, int request_id, int socket_fd, char* message) {
   if (message == NULL){
        printf("Erreur, message vide \n");    
        return -1;
    }

    printf("Client %d attempting to send %s \n",client_id,message);

    //Envoi du message
    FILE *socket_w = fdopen(socket_fd, "w");
    fprintf(socket_w, "%s", message);
    fflush(socket_w);

    //Gestion réception de la réponse
    FILE *socket_r = fdopen(socket_fd, "r");
    char* args='\0';
    size_t args_len = 0;     
    int cnt = getline(&args, &args_len, socket_r);
    printf("Client %d received %s \n", client_id, args);

    fclose(socket_w);
    fclose(socket_r);
    close(socket_fd);

    //getline renvoie parfois -1, malgré args != NULL (!)
    if (args == NULL && cnt == -1){
        perror("Erreur réception client \n");
	    if (args) {free(args);}
        return -1;
    }
 
    //On traite les choix possibles de réponse du serveur
    if (strcmp(args,"ACK \n\0") == 0){
	  if (args) {free(args);}

      //Notre requête a été acceptée, on retourne un code positif
      return 1;

    }else{

	  if (strstr(args,"ERR\0")){
         printf("Commande invalide %s \n", args);
         if (args) {free(args);}
         return -1;

	  }else if(strstr(args,"WAIT\0")){
         lockIncrementUnlock(lockCount_wait,&count_on_wait);
        
        //On transforme la réponse pour obtenir le temps d'attente         
        struct array_t_string* input = parseInput(args);
        int temps = atoi(input->data[1]);
         if (input) {delete_array_string(input);}

        //Vérification basique de la valeur envoyée
        if (temps > 0 && temps < 10000000){
            sleep(temps);
        }else{
            printf("***Temps négatif ou valeur beaucoup trop grande!!***");
        }

        //Après avoir attendu, on rétablit une connexion et on rappelle 
        //récursivement la même fonction avec le même message
        socket_fd = -2;
        while (socket_fd == -2){
           socket_fd = client_connect_server();
        }

        int resultat = send_request (client_id, request_id, 
                                       socket_fd, message);
        lockIncrementUnlock(lockReqSent,&request_sent);
        if (args) {free(args);}

        //On retourne le résultat de l'enfant le plus profond dans l'arbre
        return resultat;

	  }else{
         //On ne gère pas les autres commandes
         printf("Invalid/Unknown protocol action \n");
	     if (args) {free(args);}
         return -1;
      }
    }
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


/*
  Client établit une connexion avec le serveur
  Retourne le file descriptor ou -2 si erreur
  Basé sur https://www.thegeekstuff.com/2011/12/
  c-socket-programming/?utm_source=feedburner
*/
int client_connect_server(){
    
    int client_socket_fd = -1;

    if ((client_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("ERROR opening socket ");
        return -2;
    }

    struct hostent *hostInternet;

    if ((hostInternet = gethostbyname("localhost")) == NULL){
        perror("ERROR finding IP ");
        return -2;
    };

    struct sockaddr_in server_address;
    memset (&server_address, 0, sizeof (server_address));
    server_address.sin_family = AF_INET;
   
    server_address.sin_port = htons(port_number);
    server_address.sin_addr.s_addr = INADDR_ANY;
    
    //Padding nécessaire pour Posix
    memset (server_address.sin_zero, 0, sizeof (server_address.sin_zero));
    
    if (connect(client_socket_fd,(struct sockaddr *) &server_address, 
               sizeof(server_address)) < 0 ){

        perror("ERROR connexion ");
        return -2;
    };

    //Si aucune erreur..
    printf("** Client FD %d est connecté ** \n", client_socket_fd);
    return client_socket_fd;
}

/*
    Construit la REQ avec soit des valeurs aléatoires
    Soit une libération des valeurs allocated (pour la dernière)
*/
void make_request(client_thread* ct ){

  int temp[num_resources]; //Hold les valeurs aléatoires
  int client_socket_fd;

  for (unsigned int request_id = 0; request_id < num_request_per_client;
      request_id++){

        printf("Client %d wants to connect for REQ %d \n", ct->id, request_id);
        
        client_socket_fd = -2;
        while (client_socket_fd == -2){
            client_socket_fd = client_connect_server();
        }

        printf("Client %d connected on FD %d \n", ct->id, client_socket_fd);

        //Crée la REQ et assigne le id du client
  		char message[50]  = "REQ";
  		char append[5]; 
  		sprintf(append," %d",ct->id);
  		strcat(message,append);

        //Si la derniere requete, on doit tout libérer
        if (request_id == (num_request_per_client-1)){
            printf("+-+-+Je suis dans la dernière requête +-+-+ \n");

            for (int i = 0; i < num_resources; ++i){
                //On envoie l'opposé du nombre de ressources allocated
                int number = ct->initressources[i];
                number = number*-1;
  			    sprintf(append," %d",number); 
        	    strcat(message, append);
  		    }
        }else{
            int val;
  		    for (int i = 0; i < num_resources; ++i){
                //Si on a déjà des ressources, on peut soit en demander plus
                //Ou les libérer
                if(ct->initressources[i]>=1){
                    //Chiffre déterminant si on demande ou libère
                    int negorpos = make_random_req(provisioned_resources[i]);
                    if (negorpos<0){
                        //On libère un nombre entre 0 et ce qu'on a
                        val = make_random(ct->initressources[i]);
                        val = -1* val;
                    }else{
                        //On demande un nombre entre max - ce qu'on a
                        val = make_random(ct->initmax[i]-ct->initressources[i]);
                    }
                }else{
                    val = make_random(ct->initmax[i]-ct->initressources[i]);
                }

                //Assigne la valeur et l'ajoute au message
                temp[i] = val;
  			    sprintf(append," %d",val); 
        	    strcat(message, append);
  		    }
        }
        strcat(message, " \n");

        //Envoi de la requête
        lockIncrementUnlock(lockReqSent,&request_sent);
    	if (send_request (ct->id, request_id, client_socket_fd,message) != 1){
	    	printf("Client %d - ERROR \n", ct->id);
            lockIncrementUnlock(lockCount_inv,&count_invalid);

   		}else{
	    		printf("Client %d - ACK RECEIVED \n", ct->id);
                //Serveur a accepté notre demande, on affecte les ressources
                for (int i = 0; i < num_resources; i++){
                    ct->initressources[i] += temp[i];
                }
                lockIncrementUnlock(lockCount_acc,&count_accepted);
    	}

  }
}

/*
    Initialise les clients, envoie les requêtes et ferme les clients
*/
void* ct_code (void *param){
    int tagINI = 0;
    client_thread *ct;
    int client_socket_fd;

    //Loop jusqu'à réception d'un INI
    while(!tagINI){
        char message[25]="INI";

        //Attend une connexion valide
        client_socket_fd = -2;
        while (client_socket_fd == -2){
            client_socket_fd = client_connect_server();
        }

        //Initialise la structure
        ct = (client_thread *) param;
        
        char append[25];
        sprintf(append," %d",ct->id);
        strcat(message, append);

        //Choisit valeurs max (du INI) de façon random
        //Les ajoute de façon dynamique à la liste dans la struct client_th
        //Et au message qui sera envoyé au serveur
        int rando;
        for (int i =0; i < num_resources;i++){       
            rando = make_random(provisioned_resources[i]);
            ct->initmax[i] = rando;
            memset(append, 0, sizeof(append));
            sprintf(append," %d",rando);
            strcat(message, append);
        }
        strcat(message, " \n");


        //Envoie la requête INI
        if (send_request(ct->id,-1,client_socket_fd,message) != 1){
        	printf("Client INI %d on FD %d - ERROR \n", 
                    ct->id, client_socket_fd);
        } else{
        	printf("Client INI %d on FD %d - ACK RECEIVED \n", 
                    ct->id, client_socket_fd);
            tagINI = 1; //On sort de la loop
        }
    }

     //Va envoyer toutes les requêtes au serveur
     make_request(ct);

     printf("Client %d done requesting, need to close \n", ct->id);

     //Se reconnecte au serveur
     client_socket_fd = -2;
     while (client_socket_fd == -2){
            client_socket_fd = client_connect_server();
      }

      //Prépare le message de fermeture
      char message1[50]  = "CLO";
      char append1[5]; 
      sprintf(append1," %d",ct->id);
      strcat(message1,append1);
      strcat(message1, " \n");
      
     if(send_request (ct->id, num_request_per_client, 
        client_socket_fd,message1) != 1){

        printf("Client CLO %d on FD %d - ERROR \n", 
                ct->id, client_socket_fd); 
     
     }else{
        printf("Client CLO %d on FD %d - ACK RECEIVED \n", 
                ct->id, client_socket_fd);
     	
       lockIncrementUnlock(lockCount_disp,&count_dispatched);
     }
      

     free(ct->initressources);
     free(ct->initmax);
     return NULL;
}

/*
    Attend que toutes les requêtes ont été traitées et confirmées
*/
void ct_wait_server (){
  while(count_dispatched != num_clients);
}


/*
    Fait les demandes de mémoire nécessaires pour la structure client_thread
    Met les valeurs à 0 pour le max et les ressources allouées
*/
void ct_init (client_thread * ct){
  pthread_mutex_lock(&lockCount);
  ct->id = count++;
  pthread_mutex_unlock(&lockCount);
  ct->initressources = malloc(num_resources*sizeof(int));
  ct->initmax = malloc(num_resources*sizeof(int));
  for (int i = 0; i < num_resources; i++){    
    ct->initressources[i] = 0;   
    ct->initmax[i]=0;   
  }
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
    fprintf (fd, "Requêtes en attente: %d\n", count_on_wait);
    fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
    fprintf (fd, "Clients dispatched : %d\n", count_dispatched);
    fprintf (fd, "Requêtes envoyées: %d\n", request_sent);
  }
  else
  {
    fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
        count_invalid, count_dispatched, request_sent);
  }
}

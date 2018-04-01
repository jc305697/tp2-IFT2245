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

// Nombre de client qui se sont terminés correctement (ACC reçu en réponse à END)
unsigned int count_dispatched = 0;                      //ACK reçu en réponse à CLO plutot?

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;

pthread_mutex_t lockCount_acc;
pthread_mutex_t lockCount_wait;
pthread_mutex_t lockCount_inv;
pthread_mutex_t lockCount_disp;
pthread_mutex_t lockReqSent;

pthread_mutex_t lockCount_acc;
pthread_mutex_t lockCount_wait;
pthread_mutex_t lockCount_inv;
pthread_mutex_t lockCount_disp;
pthread_mutex_t lockReqSent;

void ct_start(){
	if (pthread_mutex_init(&lockCount_acc,NULL) != 0)
		perror("Erreur init mutex pour nombre de ack (nombre de commande executer)");
	if (pthread_mutex_init(&lockCount_wait,NULL) != 0)
		perror("Erreur init mutex pour nombre de wait ");
	if (pthread_mutex_init(&lockCount_inv,NULL) != 0)
		perror("Erreur init mutex pour nombre de requete invalide");
	if (pthread_mutex_init(&lockReqSent,NULL) != 0)
		perror("Erreur init mutex pour nombre requêtes envoyées");
	if (pthread_mutex_init(&lockCount_disp,NULL) != 0)
		perror("Erreur init mutex pour nombre de client terminé");
}


int make_random(int max_resources){
    return rand() % (max_resources+1);// fait + 1 pour povoir demander le maximum d'une ressource
}

int make_random_req(int max_resources){
    
	int nombre = make_random(max_resources);
	if (((double)rand() / (double)RAND_MAX) < 0.5)//code pris de https://stackoverflow.com/questions/6218399/how-to-generate-a-random-number-between-0-and-1
	{
        
		return nombre * -1;
	}else{
	    return nombre;
    }
}

void lockIncrUnlock(pthread_mutex_t mymutex, int count){
    pthread_mutex_lock(&mymutex);
    count+=1;
    pthread_mutex_unlock(&lockCount_acc);
}


// Vous devez modifier cette fonction pour faire l'envoie des requêtes
// Les ressources demandées par la requête doivent être choisies aléatoirement
// (sans dépasser le maximum pour le client). Elles peuvent être positives
// ou négatives.
// Assurez-vous que la dernière requête d'un client libère toute les ressources
// qu'il a jusqu'alors accumulées.
int 
send_request (int client_id, int request_id, int socket_fd, char* message) {
   if (message == NULL){
        printf("Erreur, message vide \n");    
        return -1;
    }

   if (client_id ==0 && request_id == 2){
     printf("veut send END\n" );
   }
    printf("Client %d attempting to send %s \n",client_id,message);

    FILE *socket_w = fdopen(socket_fd, "w");
    fprintf(socket_w, "%s", message);
    fflush(socket_w);
    FILE *socket_r = fdopen(socket_fd, "r");
    char* args;
    size_t args_len = 0;     
    ssize_t cnt = getline(&args, &args_len, socket_r);
    printf("Client %d received %s \n", client_id, args);
    fclose(socket_w);
    fclose(socket_r);
    close(socket_fd);
    switch (cnt) {
        case -1:
            perror("Erreur réception client \n");
            return cnt;
        default:
            break;
    }


    //struct array_t* input = parseInput(copy);
    if (strcmp(args,"ACK \n") == 0){
	  lockIncrUnlock(lockCount_acc,count_accepted);
      return 1;

    }else{
	  if (strstr(args,"ERR")){
    	 lockIncrUnlock(lockCount_inv,count_invalid);
         printf("Commande invalide %s \n", args);
         return -1;
	  }else if(strstr(args,"WAIT")){
         printf("Client sait qu'il doit WAIT \n");
         lockIncrUnlock(lockCount_wait,count_on_wait);
         //struct array_t* input = parseInput(test);
        //TODO: Trouver une manière d'aller fetch deuxieme arg san changer input
         sleep(5);
         socket_fd = -2;
         while (socket_fd == -2){
            socket_fd = client_connect_server();
         }
         int resultat = send_request (client_id, request_id, socket_fd, message);
         printf("Right after recursion \n");
         return resultat;
	  }else{
         printf("Invalid protocol action \n");
         return -1;
      }
    }
}


struct array_t_string *parseInput(char *input){
  char *token =strtok(input,"\n");
  struct array_t_string *array = new_arrayString(5);
  token = strtok(token," ");
  int i =0;
  while(token != NULL){
  	if(push_backString(array,token)==-1){
  		perror("PARSE ERROR");
  	}
  	token = strtok(NULL," ");
  	 i +=1;
  }
  return array;
}


//Basé sur https://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner
int client_connect_server(){
    //printf("Un client essaie de créer un socket \n");
    int client_socket_fd=-1;

    //Crée un socket via addresse IPV4 et TCP ou UDP
    if ((client_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("ERROR opening socket");
        return -2;
    }

    struct hostent *hostInternet;
    if ((hostInternet = gethostbyname("localhost")) == NULL){
        perror("ERROR finding IP");
        return -2;
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

    if (connect(client_socket_fd,(struct sockaddr *) &server_address, sizeof(server_address)) < 0 ){
        perror("ERROR connexion");
        return -2;
    };
    printf("** Client sur le  FD %d est connecté à un serveur ** \n", client_socket_fd);

    return client_socket_fd;
}


void make_request(client_thread* ct ){
  int temp[num_resources];
  int client_socket_fd;
  for (unsigned int request_id = 0; request_id < num_request_per_client;
      request_id++){

        printf("Client %d attempting to connect to do REQ %d \n", ct->id, request_id);
        client_socket_fd = -2;
        while (client_socket_fd == -2){
            client_socket_fd = client_connect_server();
        }
        printf("Client %d connected on FD %d \n", ct->id, client_socket_fd);
  		char message[50]  = "REQ";
  		char append[5]; 
  		sprintf(append," %d",ct->id);
  		strcat(message,append);

        //Derniere requete
        if (request_id == (num_request_per_client-1)){
            printf("+-+-+Je suis dans la dernière requête +-+-+ \n");
            for (int i = 0; i < num_resources; ++i){
                int number = ct->initressources[i];
                number = number*-1;
  			    sprintf(append," %d",number); 
        	    strcat(message, append);
  		    }
        }else{
            int val;
  		    for (int i = 0; i < num_resources; ++i){
                if(ct->initressources[i]>=1){
                    int negorpos = make_random_req(provisioned_resources[i]);
                    if (negorpos<0){
                        val = make_random(ct->initressources[i]);
                        val = -1* val;
                    }else{
                        val = make_random(ct->initmax[i]-ct->initressources[i]);
                    }
                }else{
                   
                    val = make_random(ct->initmax[i]-ct->initressources[i]);
                }
                temp[i] = val;
  			    sprintf(append," %d",val); 
        	    strcat(message, append);
  		    }
        }
        strcat(message, " \n");
    	if (send_request (ct->id, request_id, client_socket_fd,message) != 1){
	    	    	printf("Client %d - WAIT OR ERROR \n", ct->id);
                    count_invalid+=1;
                for (int i = 0; i < num_resources; i++){
                    printf("Client %d a %d \n",ct->id,ct->initressources[i]);
                }
   		 }
    	else{
	    		printf("Client %d - ACK RECEIVED \n", ct->id);
                count_accepted+=1;
                for (int i = 0; i < num_resources; i++){
                    ct->initressources[i] += temp[i];
                }
        request_sent+=1;
    	}

  }
}

void* ct_code (void *param){
    int tagINI = 0;
    client_thread *ct;
    int client_socket_fd;

    while(!tagINI){
        char message[25]="INI";
        client_socket_fd = -2;
        while (client_socket_fd == -2){
            client_socket_fd = client_connect_server();
        }
        ct = (client_thread *) param;
        //Initialise le client
 
        char append[25];
        sprintf(append," %d",ct->id); // put the int into a string
        strcat(message, append);
        int popo;
        //Choisit valeurs max de façon random
        for (int i =0; i < num_resources;i++){
            //On a au moins une ressource        
            popo = make_random(provisioned_resources[i]);
            ct->initmax[i] = popo;
            memset(append, 0, sizeof(append));
            sprintf(append," %d",popo); // put the int into a string
            strcat(message, append); // modified to append string
        }
        strcat(message, " \n");


        //Envoie la requête INI
        if (send_request(ct->id,-1,client_socket_fd,message) != 1){
        	printf("Client INI %d on FD %d - WAIT OR ERROR \n", ct->id, client_socket_fd);
        }

        else{
        	printf("Client INI %d on FD %d - ACK RECEIVED \n", ct->id, client_socket_fd);
            tagINI = 1;
        }

        //printf("Closing socket %d for Client %d \n", client_socket_fd, ct->id);
        //close(client_socket_fd);
    }
      make_request(ct);
      printf("done requesting, need to close \n");
      //ct_wait_server();
      printf("after waiting a bit \n");
      client_socket_fd = -2;
      while (client_socket_fd == -2){
            client_socket_fd = client_connect_server();
      }
      char message1[50]  = "CLO";
      char append1[5]; 
      sprintf(append1," %d",ct->id);
      strcat(message1,append1);
      strcat(message1, " \n");
      
        if(send_request (ct->id, num_request_per_client, client_socket_fd,message1) != 1){
           printf("Client CLO %d on FD %d - WAIT OR ERROR \n", ct->id, client_socket_fd);
           
         }

        else{
            printf("Client CLO %d on FD %d - ACK RECEIVED \n", ct->id, client_socket_fd);
         	pthread_mutex_lock(&lockCount_disp);
            count_dispatched++;
           	pthread_mutex_unlock(&lockCount_disp);
        }
      
        printf("Closing socket %d for Client %d \n", client_socket_fd, ct->id);
        //close(client_socket_fd);
        free(ct->initressources);
        free(ct->initmax);
        return NULL;
}

//
// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
//
void ct_wait_server (){

  //sleep(4);
  while(count_dispatched != num_clients);
}


void ct_init (client_thread * ct){
  ct->id = count++;
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

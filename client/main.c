#include "client_thread.h"
#include <ctype.h>
#define _GNU_SOURCE
int checkValue(int val);
void send_end(int socket_fd);
int checkAlpha(char* val);

/*
    Utilise la librairie ctype.h pour vérifier si val est une lettre
*/
int checkAlpha(char* val){
    if(val){
        int i=0;
        while(val[i]){
            if (isalpha(val[i])){
                return 1;
            }
            i++;
        }
    }else{
        return -1;
    }
    return 0;
}

int main (int argc, char *argv[]){
  if (argc < 5) {
    fprintf (stderr, "Usage: %s <port-nb> <nb-clients> <nb-requests> <resources>...\n",
        argv[0]);
    exit (1);
  }

  port_number = atoi (argv[1]);
  num_clients = atoi (argv[2]);
  num_request_per_client = atoi (argv[3]);
  num_resources = argc - 4;

  //----------VÉRIFICATION DES ENTRÉES------------

  // On suppose qu'on peut se connecter sur le port 0, mais pas sur un port négatif
  // Ni sur un port qui n'est pas numérique
  if((port_number == 0 && checkAlpha(argv[1]))||!checkValue(port_number)){
        printf("Le port doit être un chiffre positif.. \n");
        return -1;
  }
  
  // Puisque atoi retourne 0 si la valeur n'est pas un chiffre, on doit
  // Aussi vérifier si l'usager a vraiment entré 0
  if (num_clients == 0){
        if(checkAlpha(argv[2])){
            printf("Le nombre de clients doit être un NOMBRE \n");
        }else{
            printf("0 client, vraiment? \n");
        }
        return -1;
  }

  if(num_request_per_client == 0){
        if(checkAlpha(argv[3])){
            printf("Le nombre de requête doit être un NOMBRE \n");
        }else{
            printf("0 requête, vraiment? \n");
        }
        return -1;
  }

  // Vérifie si les valeurs sont positives
  if (!checkValue(num_clients)||!checkValue(num_request_per_client))
  {
     printf("Nombre de clients ou nombre de requêtes doit être positif \n");
     return -1;
  };


  //Assigne les ressources 
  provisioned_resources = malloc (num_resources * sizeof (int));
  for (unsigned int i = 0; i < num_resources; i++){
    int tempVal = atoi(argv[i+4]);
    //Vérifie si la valeur est une lettre ou si elle est < 0
    if (!checkValue(tempVal) || checkAlpha(argv[i+4])){
        printf("Ressources : La valeur %s est invalide \n",argv[i+4]);
        return -1;
    }
    provisioned_resources[i] = tempVal;
  }

  //----------VÉRIFICATION DES ENTRÉES TERMINÉE ------------

  int socket_test = client_connect_server();
  if (socket_test< 0){
        printf("erreur ouverture de socket\n");
        return -1;
  }

  ct_start();
  //Si on a réussi à envoyer beg et pro, on peut créer les clients
  if (send_config(socket_test)){
      client_thread *client_threads
                = malloc (num_clients * sizeof (client_thread));
        for (unsigned int i = 0; i < num_clients; i++){
            ct_init (&(client_threads[i]));
        }

        for (unsigned int i = 0; i < num_clients; i++){ 
            ct_create_and_start (&(client_threads[i]));
        }
      
      //Attend pour la synchronisation
      ct_wait_server ();

      printf("Finished sending all REQ HURRAY! \n");
      
      socket_test = client_connect_server();
      if (socket_test< 0){
        printf("erreur ouverture de socket\n");
        return -1;
      }

      send_end(socket_test);
      
      free(provisioned_resources);
      free(client_threads);

  }else{
    printf("Erreur au niveau de BEG/PRO \n");
  }


  // Affiche le journal.
  st_print_results (stdout, true);
  FILE *fp = fopen("client.log", "w");
  if (fp == NULL) {
    fprintf(stderr, "Could not print log");
    return EXIT_FAILURE;
  }
  st_print_results (fp, false);
  fclose(fp);

  return EXIT_SUCCESS;
}

int checkValue(int val){
    if (val < 0){
        printf("%d doit être >= 0 \n",val);
        return 0;
    }
    return 1;
}

/*
    Construit et envoie le message de fin du serveur
*/
void send_end(int socket_fd){
    char * message = "END \n";
    while(send_request(-1,0,socket_fd,message) != 1){
        printf("Attempting to send END \n");
    }
    printf("Server should end..\n");
}

/*
    Construit et envoie la configuration initiale (BEG et PRO)
    La première connexion est déjà établie avant la fonction
*/
bool send_config(int socket_fd){
    int retour;

    //Utilise valeur temporaire pour formatter via sprintf
    //Et recopier via strcat
    char temp[15];
    char beg[20] = "BEG ";
    sprintf(temp,"%d %d",num_resources, num_clients); 
    strcat(beg,temp);
    strcat(beg, " \n");
    char *toSend = beg;

    retour = send_request(0,0,socket_fd,toSend);
    close(socket_fd);

    //Problème avec l'envoi
    if (retour != 1 ){
      return false;
    }

    //Recrée une connexion pour le PRO
    socket_fd = client_connect_server();
    sprintf(toSend,"%s","PRO ");
    char append[50];

    //Rajoute dans le message les valeurs entrées
    for (int i=0;i<num_resources;i++){
        sprintf(append,"%d",provisioned_resources[i]);
        strcat(toSend, append);
        strcat(toSend, " ");
    }
    strcat(toSend, " \n");

    retour = send_request(0,1,socket_fd,toSend);
    close(socket_fd);

    //Problème avec l'envoi
    if (retour != 1) {
      return  false;
    }

    return true;

}



//#include <stdlib.h>
//#include <string.h>
#include "client_thread.h"
//#include <stdio.h>
#define _GNU_SOURCE
int checkValue(int val);
void send_end(int socket_fd);
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
   
  if (!checkValue(num_resources)||!checkValue(num_clients)){
     printf("Nombre de ressources ou nombre de clients invalide");
     return -1;
  };

  provisioned_resources = malloc (num_resources * sizeof (int));
  for (unsigned int i = 0; i < num_resources; i++){
    int tempVal = atoi(argv[i+4]);
    //Si argv = null ou vide, atoi retourne 0
    if (!checkValue(tempVal)){
        printf("Un valeur entrée pour les ressources et invalide");
        return -1;
    }
    provisioned_resources[i] = tempVal;
  }

  int socket_test = client_connect_server();
  ct_start();
  if (send_config(socket_test)){
      client_thread *client_threads
                = malloc (num_clients * sizeof (client_thread));
        for (unsigned int i = 0; i < num_clients; i++){
            ct_init (&(client_threads[i]));
        }

        for (unsigned int i = 0; i < num_clients; i++){ 
            ct_create_and_start (&(client_threads[i]));
        }
      //TODO: On arrive jamais à revenir ici
      ct_wait_server ();
      printf("Finished sending all REQ \n");
      
      socket_test = client_connect_server();

      send_end(socket_test);
  }else{
    printf("Erreur au niveau de BEG/PRO");
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
    if (val <= 0){
        printf("%d doit être >= 0",val);
        return 0;
    }
    return 1;
}

void send_end(int socket_fd){
    while(send_request(0,2,socket_fd,"END \n") != 1){
        printf("Attempting to send END \n");
    }
    printf("Server should end..\n");
}

bool send_config(int socket_fd){
    int retour;
    char temp[15];
    char beg[50] = "BEG ";

    sprintf(temp,"%d %d",num_resources, num_clients); 
    strcat(beg,temp);
    strcat(beg, " \n");
    char *toSend = beg;

    retour = send_request(0,0,socket_fd,toSend);
    close(socket_fd);
    if (retour != 1 ){
      return false;
    }
    //Send le pro
    socket_fd = client_connect_server();
    sprintf(toSend,"%s","PRO ");
    char append[50];
    for (int i=0;i<num_resources;i++){
        sprintf(append,"%d",provisioned_resources[i]); // put the int into a string
        strcat(toSend, append); // modified to append string
        strcat(toSend, " ");
    }
    strcat(toSend, " \n");
    retour = send_request(0,1,socket_fd,toSend);

    printf("close le socket \n");
    close(socket_fd);
    if (retour != 1)
    {
      return  false;
    }

    return true;

}

bool wait_answer(int socket_fd){
    FILE *socket_r = fdopen(socket_fd, "r");

    char *args = NULL;
    size_t args_len = 0;
    ssize_t cnt = getline(&args, &args_len, socket_r);
    switch (cnt) {
        case -1:
            perror("Erreur réception client");
            break;
        default:
            break;
    }
    printf("Ce que client a reçu %s", args);
    if (strcmp(args,"ACK")){
        return 1;
    }else{
            return 0;
    }

}



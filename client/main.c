#include <stdlib.h>
#include <string.h>
#include "client_thread.h"
#include <stdio.h>
#define _GNU_SOURCE
int main (int argc, char *argv[])
{
printf("esti de caliss");
fflush(stdout);
  if (argc < 5) {
    fprintf (stderr, "Usage: %s <port-nb> <nb-clients> <nb-requests> <resources>...\n",
        argv[0]);
    exit (1);
  }


    //TODO: Faire l'envoie parametres
    //TODO: Envoyer les requetes ensuite
        //TODO: Creer les mini INIT
        //TODO:
  port_number = atoi (argv[1]);
  int num_clients = atoi (argv[2]);
  num_request_per_client = atoi (argv[3]);
    num_resources = argc - 4;

  provisioned_resources = malloc (num_resources * sizeof (int));
  for (unsigned int i = 0; i < num_resources; i++)
    provisioned_resources[i] = atoi (argv[i + 4]);
    printf("wtf is going on");
  int socket_test = client_connect_server();
  printf("JE SUIS CONNECTÉ, JE VEUX ENVOYER \n");
  send_config(socket_test);
  bool res = wait_answer(socket_test);
    if (res){


  client_thread *client_threads
            = malloc (num_clients * sizeof (client_thread));
    for (unsigned int i = 0; i < num_clients; i++)
        ct_init (&(client_threads[i]));

    for (unsigned int i = 0; i < num_clients; i++) {
        ct_create_and_start (&(client_threads[i]));
    }


  ct_wait_server ();
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

//TODO: Pas mettre chiffres fixes
void send_config(int socket_fd){
    char temp[10];
    char beg[15] = "BEG ";
printf("Im on the edge of printing \n");
    sprintf(temp,"%d",num_resources); 
printf("i printed this shit \n");
    strcat(beg,temp);
printf("the cat has worked \n");
    char *toSend = beg;

printf("VOICI CE QUE JE VEUX SEND %s \n",toSend);
flushmoica();
    send_request(0,0,socket_fd,toSend);
    toSend = "PRO ";
printf("JE SUIS RENDU AU PRO \n");
    char append[5];
    for (int i=0;i<num_resources;i++){
        sprintf(append,"%d",provisioned_resources[num_resources]); // put the int into a string
        strcat(toSend, append); // modified to append string
        //strcat(toSend,provisioned_resources[num_resources]);
        strcat(toSend, " ");
    }
    send_request(0,1,socket_fd,toSend);
}

bool wait_answer(int socket_fd){
    FILE *socket_r = fdopen(socket_fd, "r");

    char *args = NULL;
    size_t args_len = 0;
    ssize_t cnt = getdelim(&args, &args_len, (int) ' ', socket_r);
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

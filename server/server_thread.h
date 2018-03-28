#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//POSIX library for threads
#include <pthread.h>
#include <unistd.h>
//debut code pris de fred
#include "dyn_array.h"

#include <errno.h>
//#include <malloc.h>
#ifndef _DYN_ARRAY_H
#define _DYN_ARRAY_H

#include <stddef.h>

struct array_t *new_array (size_t capacity);
void for_each(struct array_t *array, void (*callback)(void*));
int push_back(struct array_t *array, void *element);
void delete_array_callback(struct array_t **array, void (*callback)(void*));
void delete_array (struct array_t **array);

#endif
//fin premiere partie code de fred

extern bool accepting_connections;

typedef struct server_thread server_thread;
struct server_thread
{
  unsigned int id;
  pthread_t pt_tid;
  pthread_attr_t pt_attr;
};

void st_open_socket(int port_number);
void st_init (void);
void st_process_request (server_thread *, int);
void st_signal (void);
void *st_code (void *);
//void st_create_and_start(st);
void st_print_results (FILE *, bool);
void erreur(const char *message);


void sendErreur(const char *message, int socket_fd);

int st_wait();


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
#endif

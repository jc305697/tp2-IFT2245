#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//POSIX library for threads
#include <pthread.h>
#include <unistd.h>

#include <errno.h>
#include <stddef.h>


extern bool accepting_connections;

typedef struct server_thread server_thread;
struct server_thread
{
  unsigned int id;
  pthread_t pt_tid;
  pthread_attr_t pt_attr;
};

bool commEND (FILE *socket_r,FILE *socket_w);
void myFree(int **array);
void lockUnlockDestroy(pthread_mutex_t mut);
void lockIncrementUnlock(pthread_mutex_t mut, unsigned int *count);
void unlockAndDestroy(pthread_mutex_t mut);
void openAndGetline(int command, socklen_t socket_len);

void fillMatrix();

void st_open_socket(int port_number);
void st_init (void);
void st_process_request (server_thread *, int);
void st_signal (void);
void *st_code (void *);
//void st_create_and_start(st);
void st_print_results (FILE *, bool);
void erreur(const char *message);

void sendAck(FILE *socket_w, int clientTid, int req);
void sendWait(int temps,FILE *socket_w,int tid_client);
void sendErreur(const char *message, FILE *socket_w);

int st_wait();

#endif

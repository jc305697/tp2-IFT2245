//#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */
#include "server_thread.h"
//#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
#include<pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
//#include <stdbool.h>
//debut code pris de fred
#include "dyn_array.h"

//debut deuxieme partie code de fred
struct array_t {
  size_t size, capacity;
  struct Client **data;
};

struct array_t_string{
  size_t size, capacity;
  char **data;
};

struct array_t *new_array (size_t capacity) {
  struct array_t *newA = malloc(sizeof(*newA));
  if (!newA) {//erreur dans l'allocation de la mémoire
    errno = ENOMEM;
    return NULL;
  }

    newA->capacity = capacity;
    newA->size = 0;

    newA->data = malloc(capacity*sizeof(struct Client*));
  if(!newA->data) {
    free(newA);
      newA = NULL;
    errno = ENOMEM;
  }
  return newA;
}


bool estDansListe(struct array_t *liste,int clientTid ){
  if(liste){
    for (int i = 0; i < (*liste).size; ++i){

      if((liste->data)[i]->tid==clientTid){
        return true;
      }
    }
    return false;
  }

  return false;
}

void imprimeArrayString(struct array_t_string *array){
  //printf("Contenu de input\n");
  for (int i = 0; i < array->size; ++i){
    printf("%s ",array->data[i] );
  }
  printf("\n");
  //fflush(stdout);
}

struct array_t_string *new_arrayString (size_t capacity) {
  struct array_t_string *newA = malloc(sizeof(*newA));
  if (!newA) {
    errno = ENOMEM;
    return NULL;
  }

    newA->capacity = capacity;
    newA->size = 0;

    //newA->data = malloc(capacity*sizeof(char**));
    newA->data = malloc(capacity*sizeof(char *));

  if(!newA->data) {
    free(newA);
      newA = NULL;
    errno = ENOMEM;
  }
  return newA;
}

int push_back(struct array_t *array, struct Client *element) {
  if (array->size > (array->capacity - 2)) {
    size_t newsize = array->capacity << 1;
    struct Client **tmp = (struct Client **)realloc(array->data, newsize*sizeof(struct Client*));
    if (!tmp) {
      errno = ENOMEM;
      return -1;
    }
    array->capacity = newsize;
    //array->data = &tmp;
    array->data = tmp;
  }
  array->data[array->size] = element;
  array->size++;
  return 0; 
}

int push_backString(struct array_t_string *array, char *element) {
  if (array->size > (array->capacity - 2)) {
    //printf("realloue dans push_backString\n");
    size_t newsize = array->capacity << 1;
    //printf("capacity augmenter dans push_backString\n");
    char  **tmp = (char **)realloc(array->data, newsize*sizeof(char *));
   
    if (!tmp) {
      perror("pushBackString");
      errno = ENOMEM;
      return -1;
    }
    //printf("realloc reussi dans push_backString\n");
    array->capacity = newsize;
    //printf("capacity mis a jour dans push_backString\n");   
    //array->data = &tmp;
    array->data = tmp;
    //printf(" array->data mis a jour dans push_backString\n"); 
  }


  array->data[array->size] = element;
  //printf("Ajouter element dans push_backString\n");
  array->size++;
  //printf(" size mis a jour dans push_backString\n");  
  return 0; 
}

void delete_array (struct array_t *array) {
  if(array) {
    struct array_t *ptr; 
    if ((ptr = array)) {
      free(ptr->data);
      free(ptr);
    }
    array = NULL;
  }
}

void delete_array_string (struct array_t_string *array) {
  if(array) {//si array n'est pas NULL 
    //struct array_t_string *ptr; 
    //if ((ptr = array)) {
      //printf("free le data\n");
      free(array->data);
      //printf("free le pointeur\n");
      free(array);
    //}
    array = NULL; 
  }
}

//fin deuxieme partie code de fred
struct array_t deleteClientInArray(struct array_t *array, int clientTid){
  //inspire par le code de fred

  struct array_t arrayTemp = *new_array(array->size);
  for (int i = 0; i < array->size; ++i){
    struct Client *listeClient = *(*array).data;
    if(listeClient[i].tid!=clientTid){
      push_back(&arrayTemp,&(*(*array).data)[i]);
    }
  }
  return arrayTemp;
}

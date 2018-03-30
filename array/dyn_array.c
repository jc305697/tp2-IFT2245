#include "dyn_array.h"

#include <errno.h>
#include <malloc.h>

struct array_t {
  size_t size, capacity;
  int tid;
};
struct array_t *new_array (size_t capacity) {
  struct array_t *newA = malloc(sizeof(*newA));
  if (!newA) {//erreur dans l'allocation de la mémoire
    errno = ENOMEM;
    return NULL;
  }

    newA->capacity = capacity;
    newA->size = 0;
    newA->tid = 0;
    return newA;
}

size_t array_get_size(struct array_t *array) {
  return array ? array->size : 0;
}

int push_back(struct array_t *array, int tid) {
  if (array->size > (array->capacity - 2)) {
    size_t newsize = array->capacity << 1;
    array->capacity = newsize;
  }
  array->tid = tid;
  array->size++;
  return 0; 
}

void delete_array (struct array_t *array) {
  if(array) {
    struct array_t *ptr; 
    if ((ptr = array)) {
      free(ptr);
    }
    array = NULL;
  }
}

struct array_t deleteClientInArray(struct array_t *array, int clientTid){
  struct array_t arrayTemp = *new_array(array->size);
  //TODO: Adapter ceci, sait pas comment faire
  //struct int* listeClientId = *array->tid;
  /*for (int i = 0; i < array->size; ++i){
    if(listeClient[i].tid!=clientTid){
      push_back(&arrayTemp,clientTid);
    }
  }
*/
  return arrayTemp;
}


struct array_t_string{
  size_t size, capacity;
  char **data;
};


int estDansListe(struct array_t *liste,int clientTid){
    //TODO: Adapter ceci
    /*
    if(liste){
        for (int i=0;i < (*liste).size; ++i){
            if (liste[i]->tid == clientTid){
                return 1;
            }
        }
        return 0;
    }
    return 0;
*/
return 1;
}


struct array_t_string *new_arrayString (size_t capacity) {
  struct array_t_string *newA = malloc(sizeof(*newA));
  if (!newA) {
    errno = ENOMEM;
    return NULL;
  }

    newA->capacity = capacity;
    newA->size = 0;
    newA->data = malloc(capacity*sizeof(char *));

  if(!newA->data) {
    free(newA);
      newA = NULL;
    errno = ENOMEM;
  }
  return newA;
}


int push_backString(struct array_t_string *array, char *element) {
  if (array->size > (array->capacity - 2)) {
    size_t newsize = array->capacity << 1;
    char  **tmp = (char **)realloc(array->data, newsize*sizeof(char *));
   
    if (!tmp) {
      perror("pushBackString");
      errno = ENOMEM;
      return -1;
    }
    array->capacity = newsize;
    array->data = tmp;
  }


  array->data[array->size] = element;
  array->size++;
  return 0; 
}


void delete_array_string (struct array_t_string *array) {
  if(array) {//si array n'est pas NULL 
      free(array->data);
      free(array);
    array = NULL; 
  }
}

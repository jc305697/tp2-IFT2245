#ifndef _DYN_ARRAY_H
#define _DYN_ARRAY_H

#include <stddef.h>

struct array_t *new_array (size_t capacity);
struct array_t;
struct array_t_string;
int estDansListe(struct array_t *liste,int clientTid);
size_t array_get_size(struct array_t *array);
void **array_get_data(struct array_t *array);
void delete_array_callback(struct array_t **array, void (*callback)(void*));
void delete_array (struct array_t *array);

void for_each(struct array_t *array, void (*callback)(void*));
int push_back(struct array_t *array, int tid);
void delete_array_string (struct array_t_string *array);
int push_backString(struct array_t_string *array, char *element);
struct array_t_string *new_arrayString (size_t capacity);
struct array_t deleteClientInArray(struct array_t *array, int clientTid);

#endif

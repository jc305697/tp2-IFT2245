#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Include la library dynamic array
#include "dyn_array.h"

int main(int argc, char **argv) {
  // Create a matrix 3x4
  const int NB_COLUMNS = 4;
  const int NB_LINE = 3;
  struct array_t *array = new_array(NB_LINE);

  // Extract inner_data
  int **data;
  int *new_line;

  srandom(time(NULL));
  for(int i = 0; i < NB_LINE; ++i) {
    new_line = malloc(NB_COLUMNS);
    for(int j = 0; j < NB_COLUMNS; ++j) {
      new_line[j] = random() % 64;
    }
    push_back(array, new_line);
  }

  data = (int**)array_get_data(array);
  for (int i = 0; i < NB_LINE; ++i) {
    for(int j = 0; j < NB_COLUMNS; ++j) {
      printf("%3d", data[i][j]);
    }
    putchar('\n');
  }

  delete_array(&array);
  return 0;
}

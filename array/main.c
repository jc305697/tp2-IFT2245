
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_array.h"

int is_number(char *tok) {
  int len = 0;
  if (tok && (*tok == '-')) {
    ++tok;
  }
  while(tok && isdigit(*tok)) {
    ++tok;
    ++len;
  }
  return len > 0 && len < 10;
}

void println(void *data) {
  printf("Argument %s\n", (char*)data);
}

int main(int argc, char **argv) {
  struct array_t *array = new_array(5);
  char *line = NULL;
  size_t size = 0;
  ssize_t len;
  for(;;) {
    printf("> ");
    if ((len=getline(&line, &size, stdin)) < 0) {
      free(line);
      break;
    }

    char *tok = strtok(line, " ");
    while(tok) {
      push_back(array, strdup(tok));
      tok = strtok(NULL, " ");
    }
  }
  for_each(array, println);
  delete_array_callback(&array, free);
  return 0;
}


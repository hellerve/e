#include <stdlib.h>
#include <string.h>

#ifndef ABUF_INIT
typedef struct {
  char* b;
  int len;
} append_buf;

#define ABUF_INIT {NULL, 0}
#endif

void ab_append(append_buf*, const char*, int);
void ab_free(append_buf*);

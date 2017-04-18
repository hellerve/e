#include <stdlib.h>
#include <string.h>

#include "util.h"

#ifndef ABUF_INIT
typedef struct {
  wchar_t* b;
  int len;
} append_buf;

#define ABUF_INIT {NULL, 0}
#endif

void ab_append(append_buf*, const wchar_t*, int);
void ab_free(append_buf*);

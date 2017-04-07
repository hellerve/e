#include "buffer.h"

void ab_append(append_buf* ab, const char* s, int len) {
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void ab_free(append_buf* ab) {
  free(ab->b);
}

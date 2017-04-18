#include "buffer.h"

void ab_append(append_buf* ab, const wchar_t* s, int len) {
  wchar_t *new = realloc(ab->b, (ab->len + len) * sizeof(wchar_t));
  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void ab_free(append_buf* ab) {
  free(ab->b);
}

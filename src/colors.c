#include "colors.h"

void color_append(int color, append_buf* ab, const char* str, int len) {
  char colbuf[4];
  snprintf(colbuf, 4, "%dm", color);

  ansi_append(ab, colbuf, strlen(colbuf));
  ab_append(ab, str, len);
}

void ansi_append(append_buf* ab, const char* code, int len) {
  ab_append(ab, "\x1b[", 2);
  ab_append(ab, code, len);
}

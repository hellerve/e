#include "colors.h"

void color_append(int color, append_buf* ab, char* str, int len) {
  char colbuf[3];
  snprintf(colbuf, 3, "%d", color);

  int comb_len = strlen(colbuf) + len + 8;
  char buf[comb_len];
  snprintf(buf, comb_len, "\x1b[%sm%s\x1b[0m", colbuf, str);
  ab_append(ab, buf, comb_len);
}

#include "colors.h"

void color_write(int color, int fd, char* str) {
  char colbuf[3];
  snprintf(colbuf, 3, "%d", color);

  dprintf(fd, "\x1b[%sm%s\x1b[0m", colbuf, str);
}

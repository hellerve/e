#include <stdio.h>

enum {
  NORMAL,
  BOLD,

  UNDERLINE = 4,
  BLINK,

  REVERSE = 7,
  INVISIBLE,

  BLACK = 30,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN,
  WHITE,

  BLACK_BG = 40,
  RED_BG = 41,
  GREEN_BG = 42,
  YELLOW_BG = 43,
  BLUE_BG = 44,
  MAGENTA_BG = 45,
  CYAN_BG = 46,
  WHITE_BG = 47
};

void color_write(int color, int fd, char* str);

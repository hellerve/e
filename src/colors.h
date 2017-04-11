#include <stdio.h>

#include "buffer.h"

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

enum {
  HL_NORMAL=0,
  HL_NUM,
  HL_STRING,
  HL_COMMENT,
  HL_KEYWORD,
  HL_TYPE,
  HL_PRAGMA,
  HL_PREDEF,

  HL_MATCH,
};


void color_append(int, append_buf*, const char*, int);
void ansi_append(append_buf*, const char*, int);
int syntax_to_color(char);

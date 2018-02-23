#include <stdio.h>

#include "buffer.h"
#include "util.h"

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
  RED_BG,
  GREEN_BG,
  YELLOW_BG,
  BLUE_BG,
  MAGENTA_BG,
  CYAN_BG,
  WHITE_BG
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
  HL_TODO,
};


void color_append(int, append_buf*, const char*, int);
void ansi_append(append_buf*, const char*, int);
int syntax_to_color(char);

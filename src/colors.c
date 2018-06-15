#include "colors.h"

void color_append(int color, append_buf* ab, const char* str, int len) {
  /* TODO: this is an ugly special case needed by UTF-8 strings */
  if (len == 1 && isutf8cont(str[0])) {
    ab_append(ab, str, len);
    return;
  }
  char colbuf[20];

  if (color > 50) snprintf(colbuf, 20, "38;5;%dm", color);
  else snprintf(colbuf, 20, "%dm", color);

  ansi_append(ab, colbuf, strlen(colbuf));
  ab_append(ab, str, len);
}

void ansi_append(append_buf* ab, const char* code, int len) {
  ab_append(ab, "\x1b[", 2);
  ab_append(ab, code, len);
}

int syntax_to_color(char hl) {
  switch (hl) {
    case HL_NUM: return RED;
    case HL_MATCH: return BLUE_BG;
    case HL_STRING: return ORANGE;
    case HL_COMMENT: return CYAN;
    case HL_KEYWORD: return YELLOW;
    case HL_TYPE: return GREEN;
    case HL_PRAGMA: return MAGENTA;
    case HL_PREDEF: return RED;
    case HL_TODO: return YELLOW_BG;
    default: return NORMAL;
  }
}

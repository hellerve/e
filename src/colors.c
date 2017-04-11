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

int syntax_to_color(char hl) {
  switch (hl) {
    case HL_NUM: return RED;
    case HL_MATCH: return BLUE_BG;
    case HL_STRING: return MAGENTA;
    case HL_COMMENT: return CYAN;
    case HL_KEYWORD: return YELLOW;
    case HL_TYPE: return GREEN;
    case HL_PRAGMA: return MAGENTA;
    case HL_PREDEF: return RED;
    default: return NORMAL;
  }
}

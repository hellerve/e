#include "util.h"

int isnum(char* str) {
  if (!str) return 0;

  if (*str == '-') str++;

  if (!str) return 0;

  while (*str) {
    if (!isdigit(*str)) return 0;
    str++;
  }
  return 1;
}

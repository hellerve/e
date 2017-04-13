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


int issep(char c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}


char fpeek(FILE *f) {
    char c;

    c = fgetc(f);
    ungetc(c, f);

    return c;
}


char *strsub(char* str, char *pat, char *sub) {
    char* res;
    char* ins;
    char* tmp;
    int lpat;
    int lsub;
    int lfront;
    int i;

    if (!str || !pat) return NULL;

    lpat = strlen(pat);
    if (!lpat) return NULL;
    if (!sub) sub = (char*) "";

    lsub = strlen(sub);

    ins = str;
    for (i = 0; (tmp = strstr(ins, pat)); ++i) ins = tmp + lpat;
    if (!i) return NULL;

    tmp = res = malloc(strlen(str) + (lsub - lpat) * i + 1);

    if (!res) return NULL;

    while (i--) {
        ins = strstr(str, pat);
        lfront = ins - str;
        tmp = strncpy(tmp, str, lfront) + lfront;
        tmp = strcpy(tmp, sub) + lsub;
        str += lfront + lpat;
    }
    strcpy(tmp, str);
    return res;
}

char* strtriml(char* str) {
  while(isspace((unsigned char)*str)) str++;

  return str;
}

int strcmpr(char* str, char* suffix) {
    size_t strl;
    size_t suffixl;

    if (!str || !suffix) return 0;

    strl = strlen(str);
    suffixl = strlen(suffix);

    if (suffixl > strl) return 0;
    return strncmp(str+strl-suffixl, suffix, suffixl) == 0;
}

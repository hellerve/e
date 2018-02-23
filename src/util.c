#include "util.h"

int utf8len(const char *s) {
  int len = 0;
  while(*s) len += (*(s++)&0xC0)!=0x80;
  return len;
}

int utf8len_to(const char *s, int to) {
  int len = 0;
  int i = 0;
  while(*s && i++ < to) len += (*(s++)&0xC0)!=0x80;
  return len;
}

short isutf8cont(char c) {
  return (c&0xC0)==0x80;
}

short isnum(char* str) {
  if (!str) return 0;

  if (*str == '-') str++;

  if (!str) return 0;

  while (*str) {
    if (!isdigit(*str)) return 0;
    str++;
  }
  return 1;
}


short issep(char c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];:", c) != NULL;
}


char fpeek(FILE *f) {
  char c;

  c = fgetc(f);
  ungetc(c, f);

  return c;
}


char* strsub(char* str, const char *pat, const char *sub) {
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


void msleep(int ms) {
#ifdef WIN32
  Sleep(ms);
#else
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
#endif
}

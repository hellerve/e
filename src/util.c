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


int issep(wchar_t c) {
  return iswspace(c) || c == '\0' || wcschr(L",.()+-/*=~%<>[];:", c) != NULL;
}


char fpeek(FILE *f) {
    char c;

    c = fgetc(f);
    ungetc(c, f);

    return c;
}


wchar_t* strsub(wchar_t* str, wchar_t* pat, wchar_t* sub) {
    wchar_t* res;
    wchar_t* ins;
    wchar_t* tmp;
    int lpat;
    int lsub;
    int lfront;
    int i;

    if (!str || !pat) return NULL;

    lpat = wcslen(pat);
    if (!lpat) return NULL;
    if (!sub) sub = (wchar_t*) L"";

    lsub = wcslen(sub);

    ins = str;
    for (i = 0; (tmp = wcsstr(ins, pat)); ++i) ins = tmp + lpat;
    if (!i) return NULL;

    tmp = res = malloc((wcslen(str) + (lsub - lpat) * i + 1)*sizeof(wchar_t));

    if (!res) return NULL;

    while (i--) {
        ins = wcsstr(str, pat);
        lfront = ins - str;
        tmp = wcsncpy(tmp, str, lfront) + lfront;
        tmp = wcscpy(tmp, sub) + lsub;
        str += lfront + lpat;
    }
    wcscpy(tmp, str);
    return res;
}


char* strtriml(char* str) {
  while(isspace(*str)) str++;

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

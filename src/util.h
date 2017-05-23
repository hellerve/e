#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>

#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif

int isnum(char*);
int issep(wchar_t);
wchar_t* strsub(wchar_t*, wchar_t*, wchar_t*);
char* strtriml(char*);
int strcmpr(char*, char*);
char fpeek(FILE*);
void msleep(int);
size_t getwline(wchar_t** const, size_t* const, FILE* const);

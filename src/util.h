#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif

int utf8len(const char*);
int utf8len_to(const char*, int);
short isutf8cont(char);
short isnum(char*);
short issep(char);
char* strsub(char*, char*, char*);
char* strtriml(char*);
int strcmpr(char*, char*);
char fpeek(FILE*);
void msleep(int);

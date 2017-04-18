#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif

int isnum(char*);
int issep(char);
char* strsub(char*, char*, char*);
char* strtriml(char*);
int strcmpr(char*, char*);
char fpeek(FILE*);
void msleep(int);

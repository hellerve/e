#include <regex.h>
#include <stdlib.h>

#include "colors.h"

#define HL_NUMS (1<<0)
#define HL_STRINGS (1<<1)
#define HL_COMMENTS (1<<2)

typedef struct pattern {
  regex_t pattern;
  regex_t closing;
  int color;
  char needs_sep;
  char multiline;
} pattern;

typedef struct syntax {
  char* ftype;
  int matchlen;
  regex_t* filematch;
  int flags;
  int npatterns;
  pattern* patterns;
} syntax;

syntax** syntax_init(char*);
void syntax_free(syntax**);

#include "syntax.h"

int syntax_lookup_color(char* key) {
  if (!strncmp(key, "number", 6)) return HL_NUM;
  else if (!strncmp(key, "string", 6)) return HL_STRING;
  else if (!strncmp(key, "string", 6)) return HL_STRING;
  else if (!strncmp(key, "comment", 7)) return HL_COMMENT;
  else if (!strncmp(key, "keyword", 7)) return HL_KEYWORD;
  else if (!strncmp(key, "type", 4)) return HL_TYPE;
  else if (!strncmp(key, "pragma", 6)) return HL_PRAGMA;
  else if (!strncmp(key, "predefined", 10)) return HL_PREDEF;
  else if (!strncmp(key, "match", 5)) return HL_MATCH;
  return HL_NORMAL;
}


void syntax_read_extensions(syntax* c, FILE* f, char* line) {
  int regl = 1;
  size_t ln;
  regex_t* reg = malloc(sizeof(regex_t));
  int err;

  regcomp(reg, line, REG_EXTENDED);

  while (fpeek(f) == ' ' || fpeek(f) == '\t') {
    fgets(line, MAX_LINE_WIDTH, f);
    ln = strlen(line)-1;
    line[ln] = '\0'; // replace newline
    reg = realloc(reg, sizeof(regex_t) * ++regl);
    err = regcomp(&reg[regl-1], strtriml(line), REG_EXTENDED);
    if (err) exit(err);
  }
  c->matchlen = regl;
  c->filematch = reg;
}


void syntax_read_pattern(syntax* c, FILE* f, char* key, char* value) {
  pattern* pat = malloc(sizeof(pattern));
  char line[MAX_LINE_WIDTH];
  size_t ln;
  int err;
  key = strtok(key, "|");
  char* no_sep = strtok(NULL, "|");
  pat->color = syntax_lookup_color(key);
  pat->needs_sep = no_sep && !strncmp(no_sep, "no_sep", 6);
  pat->multiline = 0;
  ln = strlen(value);
  memmove(value+1, value, ln);
  value[0] = '^';
  err = regcomp(&pat->pattern, value, REG_EXTENDED);
  if (err) exit(err);

  if (fpeek(f) == ' ' || fpeek(f) == '\t') {
    fgets(line, MAX_LINE_WIDTH, f);
    char* l = strtriml(line);
    ln = strlen(l)-1;
    memmove(l+1, l, ln);
    l[0] = '^';
    err = regcomp(&pat->closing, line, REG_EXTENDED);
    if (err) exit(err);
    pat->multiline = 1;
  }

  c->npatterns++;
  c->patterns = realloc(c->patterns, sizeof(pattern)*c->npatterns);
}


syntax* syntax_read_file(char* fname) {
  FILE* f;
  syntax* c;
  char* key;
  char* value;
  char* line = malloc(MAX_LINE_WIDTH);
  size_t ln;

  f = fopen(fname, "r");

  if (!f) return NULL;

  c = malloc(sizeof(syntax));
  c->patterns = NULL;
  c->npatterns = 0;

  while (fgets(line, MAX_LINE_WIDTH, f)) {
    ln = strlen(line)-1;
    line[ln] = '\0'; // replace newline
    key = strtok(line, ":");
    value = strtok(NULL, ":");
    if (!strncmp(key, "displayname", 11)) {
      c->ftype = strdup(strtriml(value));
    } else if (!strncmp(key, "extensions", 10)) {
      syntax_read_extensions(c, f, strtriml(value));
    } else {
      if (value) syntax_read_pattern(c, f, key, strtriml(value));
    }
  }

  return c;
}


syntax** syntax_init(char* dir) {
  int retl = 1;
  DIR* dp = opendir(dir);
  char fname[MAX_LINE_WIDTH];
  struct dirent* ep;
  syntax** ret = malloc(sizeof(syntax*));
  syntax* c;

  if (!dp) return NULL;

  while ((ep = readdir(dp))) {
    if (!strcmpr(fname, (char*) ".stx")) continue;
    snprintf(fname, sizeof(fname), "%s%s", dir, ep->d_name);
    c = syntax_read_file(fname);
    if (!c) continue;
    ret = realloc(ret, sizeof(syntax*)*(retl-1));
    ret[retl-1] = c;
    retl++;
  }
  ret[retl-1] = NULL;
  return ret;
}


void syntax_free(syntax** stx) {
  int i = 0;
  int j;
  while (stx[i]) {
    syntax* s = stx[i];

    for (j = 0; j < s->matchlen; j++) regfree(&s->filematch[j]);

    for (j = 0; j < s->npatterns; j++) regfree(&s->patterns[j].pattern);

    free(s);

    i++;
  }
}

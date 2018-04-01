#include "syntax.h"

void compile_err(int err, char* pat, regex_t* rx, char* name, int line) {
  char estr[80];
  int l = regerror(err, rx, estr, 80);
  if (l > 80) return;
  fprintf(stderr,
          "Syntax: encountered an error in pattern \"%s\" (%s:%d): %s\n",
          pat, name, line, estr);
  return;
}

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

int syntax_read_extensions(syntax* c, FILE* f, char* name, int lineno,
                           char* line) {
  int regl = 1;
  size_t ln;
  regex_t* reg = malloc(sizeof(regex_t));
  int err;

  regcomp(reg, line, REG_EXTENDED);

  while (fpeek(f) == ' ' || fpeek(f) == '\t') {
    if (fgets(line, MAX_LINE_WIDTH, f)) {
      ln = strlen(line)-1;
      line[ln] = '\0'; // replace newline
      line = strtriml(line);
      reg = realloc(reg, sizeof(regex_t) * ++regl);
      err = regcomp(&reg[regl-1], line, REG_EXTENDED);
      if (err) {
        compile_err(err, line, &reg[regl-1], name, lineno);

        for (int i = 0; i < regl; i++) regfree(&reg[regl]);
        free(reg);

        return 0;
      }
    }
  }
  c->matchlen = regl;
  c->filematch = reg;
  return 0;
}


int syntax_read_pattern(syntax* c, FILE* f, char* name, int lineno,
                        char* key, char* value) {
  pattern* pat = malloc(sizeof(pattern));
  char line[MAX_LINE_WIDTH];
  size_t ln;
  int err;
  key = strtok(key, "|");
  char* no_sep = strtok(NULL, "|");
  pat->color = syntax_lookup_color(key);
  pat->needs_sep = !(no_sep && !strncmp(no_sep, "no_sep", 6));
  pat->multiline = 0;
  ln = strlen(value);
  memmove(value+1, value, ln);
  value[0] = '^';
  err = regcomp(&pat->pattern, value, REG_EXTENDED);
  if (err) {
    compile_err(err, value, &pat->pattern, name, lineno);
    regfree(&pat->pattern);
    free(pat);
    return 1;
  }

  if ((fpeek(f) == ' ' || fpeek(f) == '\t') && fgets(line, MAX_LINE_WIDTH, f)) {
    char* l = strtriml(line);
    ln = strlen(l)-1;
    memmove(l+1, l, ln);
    l[0] = '^';
    err = regcomp(&pat->closing, l, REG_EXTENDED);
    if (err) {
      compile_err(err, l, &pat->closing, name, lineno);
      regfree(&pat->pattern);
      regfree(&pat->closing);
      free(pat);
      return 1;
    }
    pat->multiline = 1;
  }

  c->npatterns++;
  c->patterns = realloc(c->patterns, sizeof(pattern)*c->npatterns);
  memmove(&c->patterns[c->npatterns-1], pat, sizeof(pattern));
  free(pat);
  return 0;
}


syntax* syntax_read_file(char* fname) {
  FILE* f;
  syntax* c;
  char* key;
  char* value;
  char* line = malloc(MAX_LINE_WIDTH);
  size_t ln;
  int lineno = 0;

  f = fopen(fname, "r");

  if (!f) return NULL;

  c = malloc(sizeof(syntax));
  c->patterns = NULL;
  c->ftype = NULL;
  c->filematch = NULL;
  c->npatterns = 0;

  while (fgets(line, MAX_LINE_WIDTH, f)) {
    ln = strlen(line)-1;
    if (ln == -1) continue;
    lineno++;
    line[ln] = '\0'; // replace newline
    key = strtok(line, ":");
    value = strtok(NULL, ":");
    if (!strncmp(key, "displayname", 11)) {
      c->ftype = strdup(strtriml(value));
    } else if (!strncmp(key, "extensions", 10)) {
      if (syntax_read_extensions(c, f, fname, lineno, strtriml(value))) {
        fclose(f);
        free(line);
        syntax_free(c);
        return NULL;
      }
    } else {
      if (value) {
        if (syntax_read_pattern(c, f, fname, lineno, key, strtriml(value))) {
          fclose(f);
          free(line);
          syntax_free(c);
          return NULL;
        }
      }
    }
  }

  fclose(f);
  free(line);

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
    if (!strcmpr(ep->d_name, (char*) ".stx")) continue;
    snprintf(fname, sizeof(fname), "%s/%s", dir, ep->d_name);
    c = syntax_read_file(fname);
    if (!c) {
      ret = realloc(ret, sizeof(syntax*)*retl);
      ret[retl-1] = NULL;
      syntaxes_free(ret);
      closedir(dp);
      return NULL;
    }
    ret = realloc(ret, sizeof(syntax*)*retl);
    ret[retl-1] = c;
    retl++;
  }

  closedir(dp);
  ret = realloc(ret, sizeof(syntax*)*retl);
  ret[retl-1] = NULL;
  return ret;
}

void syntax_free(syntax* s) {
  int i;
  if (s->ftype) free(s->ftype);

  if (s->filematch) {
    for (i = 0; i < s->matchlen; i++) regfree(&s->filematch[i]);
    free(s->filematch);
  }

  for (i = 0; i < s->npatterns; i++) {
    regfree(&s->patterns[i].pattern);
    if (s->patterns[i].multiline) regfree(&s->patterns[i].closing);
  }
  free(s->patterns);

  free(s);
}

void syntaxes_free(syntax** stx) {
  int i = 0;
  while (stx[i]) syntax_free(stx[i++]);
  free(stx);
}

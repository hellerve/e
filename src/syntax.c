#include "syntax.h"

syntax** syntax_init(char* dir) {
  regex_t* fmatch = malloc(sizeof(regex_t) * 4);
  syntax** ret = calloc(2, sizeof(syntax*));
  syntax* c = malloc(sizeof(syntax));
  pattern* p = malloc(sizeof(pattern)*7);

  regcomp(&fmatch[0], ".*\\.c$", REG_EXTENDED);
  regcomp(&fmatch[1], ".*\\.h$", REG_EXTENDED);
  regcomp(&fmatch[2], ".*\\.cpp$", REG_EXTENDED);
  regcomp(&fmatch[3], ".*\\.hpp$", REG_EXTENDED);

  regcomp(&p->pattern, "^//.*$", REG_EXTENDED);
  p->color = HL_COMMENT;
  p->needs_sep = 0;
  p->multiline = 0;
  regcomp(&p[1].pattern, "^(restrict|switch|if|while|for|break|continue|return|else|try|catch|else|struct|union|class|typedef|static|enum|case|asm|default|delete|do|explicit|export|extern|inline|namespace|new|public|private|protected|sizeof|template|this|typedef|typeid|typename|using|virtual|friend|goto)", REG_EXTENDED);
  p[1].color = HL_KEYWORD;
  p[1].needs_sep = 1;
  p[1].multiline = 0;
  regcomp(&p[2].pattern, "^(auto|bool|char|const|double|float|inline|int|mutable|register|short|unsigned|volatile|void|int8_t|int16_t|int32_t|int64_t|uint8_t|uint16_t|uint32_t|uint64_t|size_t|ssize_t|time_t)", REG_EXTENDED);
  p[2].color = HL_TYPE;
  p[2].needs_sep = 1;
  p[2].multiline = 0;
  regcomp(&p[3].pattern, "^/\\*([^(\\*/)]*)?", REG_EXTENDED);
  p[3].color = HL_COMMENT;
  p[3].needs_sep = 0;
  p[3].multiline = 1;
  regcomp(&p[3].closing, "^(.*)?\\*/", REG_EXTENDED);
  regcomp(&p[4].pattern, "^#(include|pragma|define|undef) .*$", REG_EXTENDED);
  p[4].color = HL_PRAGMA;
  p[4].needs_sep = 1;
  p[4].multiline = 0;
  regcomp(&p[5].pattern, "^(NULL|stdout|stderr)", REG_EXTENDED);
  p[5].color = HL_PREDEF;
  p[5].needs_sep = 1;
  p[5].multiline = 0;
  regcomp(&p[6].pattern, "^#(ifdef|ifndef|if) .*$", REG_EXTENDED);
  p[6].color = HL_PRAGMA;
  p[6].needs_sep = 1;
  p[6].multiline = 1;
  regcomp(&p[6].closing, "^#endif\\w*$", REG_EXTENDED);

  c->ftype = (char*) "c";
  c->matchlen = 4;
  c->filematch = fmatch;
  c->flags = HL_NUMS | HL_STRINGS | HL_COMMENTS;
  c->npatterns = 7;
  c->patterns = p;

  ret[0] = c;
  ret[1] = NULL;
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

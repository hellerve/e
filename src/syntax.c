#include "syntax.h"

syntax** syntax_init(char* dir) {
  regex_t* fmatch = malloc(sizeof(regex_t) * 4);
  syntax** ret = calloc(2, sizeof(syntax*));
  syntax* c = malloc(sizeof(syntax));
  pattern* p = malloc(sizeof(pattern)*5);

  regcomp(&fmatch[0], ".*\\.c$", REG_EXTENDED);
  regcomp(&fmatch[1], ".*\\.h$", REG_EXTENDED);
  regcomp(&fmatch[2], ".*\\.cpp$", REG_EXTENDED);
  regcomp(&fmatch[3], ".*\\.hpp$", REG_EXTENDED);

  regcomp(&p->pattern, "^//.*$", REG_EXTENDED);
  p->color = HL_COMMENT;
  p->needs_sep = 0;
  p->multiline = 0;
  regcomp(&p[1].pattern, "^(switch|if|while|for|break|continue|return|else|try|catch|else|struct|union|class|typedef|static|enum|case|asm|default|delete|do|explicit|export|extern|inline|namespace|new|public|private|protected|sizeof|template|this|typedef|typeid|typename|using|virtual|friend|goto)", REG_EXTENDED);
  p[1].color = HL_KEYWORD;
  p[1].needs_sep = 1;
  p[0].multiline = 0;
  regcomp(&p[2].pattern, "^(auto|bool|char|const|double|float|inline|int|mutable|register|short|unsigned|volatile|void)", REG_EXTENDED);
  p[2].color = HL_TYPE;
  p[2].needs_sep = 1;
  p[2].multiline = 0;
  regcomp(&p[3].pattern, "^/\\*([^(\\*/)]*)?", REG_EXTENDED);
  p[3].color = HL_COMMENT;
  p[3].needs_sep = 0;
  p[3].multiline = 1;
  regcomp(&p[3].closing, "^(.*)?\\*/", REG_EXTENDED);

  c->ftype = (char*) "c";
  c->matchlen = 4;
  c->filematch = fmatch;
  c->flags = HL_NUMS | HL_STRINGS | HL_COMMENTS;
  c->npatterns = 4;
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

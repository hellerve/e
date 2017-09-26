#include "editor.h"

void disable_raw_mode(e_context* ctx) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &ctx->orig) == -1) e_die("tcsetattr");
}


void enable_raw_mode(e_context* ctx) {
  struct termios raw;

  raw = ctx->orig;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) e_die("tcsetattr");
}


/* sigh */
void write_wrapped(int file, const char* str, int len) {
  ssize_t x = write(file, str, len);
  (void) x;
}


void e_correct_for_fullwidth(e_context* ctx, append_buf* ab) {
  int i;
  char buf[5];
  regex_t r;
  e_row row;
  char* tmp;
  char* cur;
  regmatch_t m;

  i = 0;
  row = ctx->row[ctx->cy];
  tmp = calloc(ctx->cx+1, 1);
  memcpy(tmp, row.str, ctx->cx);
  cur = tmp;
  regcomp(&r, "[\uff00-\uff5e\u30a0-\u30ff\u3040-\u309f]", REG_EXTENDED);

  while (regexec(&r, cur, 1, &m, 0) != REG_NOMATCH) {
    i += 1;
    cur += m.rm_eo+2;
  }

  free(tmp);

  if(!i) return;

  i = snprintf(buf, 5, "%dC", i);
  ansi_append(ab, buf, i);
}


void e_die(const char* s) {
  write_wrapped(STDOUT_FILENO, "\x1b[2J\x1b[?47l\x1b""8", 12);
  perror(s);
  exit(1);
}


void e_exit() {
  write_wrapped(STDOUT_FILENO, "\x1b[2J\x1b[?47l\x1b""8", 12);
  exit(0);
}


void e_exit_prompt(e_context* ctx) {
  if (ctx->dirty) {
    e_set_status_msg(ctx, "Unsaved content! Type :! to force or :s to save-quit.");
    return;
  }
  e_exit();
}


void e_draw_rows(e_context* ctx, append_buf* ab) {
  int i;
  int h;
  int filerow;

  for (h = 0; h < ctx->rows; h++) {
    filerow = h + ctx->roff;
    if (filerow >= ctx->nrows) {
      color_append(BLUE, ab, "~\x1b[K", 4);
      if (h == ctx->rows / 3 && !ctx->nrows) {
        char welcome[80];
        int len = snprintf(welcome, sizeof(welcome),
                           "E -- braindead editing -- version %s",
                           E_VERSION);
        if (len > ctx->cols) len = ctx->cols;
        int padding = (ctx->cols - len) / 2;
        while (padding--) ab_append(ab, " ", 1);
        color_append(NORMAL, ab, welcome, len);
      }
    } else {
      ansi_append(ab, "K", 1);
      int len = ctx->row[filerow].rsize - ctx->coff;
      if (len < 0) len = 0;
      if (len > ctx->cols) len = ctx->cols;
      char* c = &ctx->row[filerow].render[ctx->coff];
      char* hl = &ctx->row[filerow].hl[ctx->coff];
      char current_color = 0;
      for (i = 0; i < len; i++) {
        if (iscntrl(c[i])) {
          char sym = (c[i] <= 26) ? '@' + c[i] : '?';
          color_append(GREEN, ab, &sym, 1);
        } else if (hl[i] == current_color) {
          ab_append(ab, &c[i], 1);
        } else {
          int color = syntax_to_color(hl[i]);
          color_append(color, ab, &c[i], 1);
          current_color = color;
        }
        if (i+1 < len && !isutf8cont(c[i+1])) color_append(NORMAL, ab, "", 0);
      }
      color_append(NORMAL, ab, "", 0);
    }
    ab_append(ab, "\r\n", 2);
  }
}


void e_draw_status(e_context* ctx, append_buf* ab) {
  if (ctx->mode == EDIT) {
    color_append(YELLOW_BG, ab, "", 0);
    color_append(BLACK, ab, "EDIT mode ", 10);
  } else if (ctx->mode == INITIAL) {
    color_append(BLUE_BG, ab, "", 0);
    color_append(WHITE, ab, "INIT mode ", 10);
  }

  color_append(BLACK_BG, ab, " ", 1);
  color_append(WHITE, ab, "", 0);
  char status[100];
  char rstatus[100];
  int len  = snprintf(status, sizeof(status), "%.40s - %d lines %s",
                      ctx->filename ? ctx->filename : "[No Name]", ctx->nrows,
                      ctx->dirty ? "[UNSAVED]" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%s  |  %d/%d  %d ",
                      ctx->stx ? ctx->stx->ftype : "unknown filetype",
                      ctx->cy+1, ctx->nrows,
                      utf8len_to(ctx->row[ctx->cy].render, ctx->cx)+1);
  color_append(WHITE, ab, status, len);
  len += 12;

  while (len < ctx->cols) {
    if (ctx->cols-len == rlen) {
      color_append(YELLOW_BG, ab, " ", 1);
      color_append(BLACK, ab, rstatus, rlen);
      break;
    }
    ab_append(ab, " ", 1);
    len++;
  }
  color_append(NORMAL, ab, "", 0);
  ab_append(ab, "\r\n", 2);
}


void e_draw_message(e_context* ctx, append_buf* ab) {
  ansi_append(ab, "K", 1);
  int len = strlen(ctx->statusmsg);
  if (len > ctx->cols) len = ctx->cols;
  if (len && time(NULL) - ctx->statusmsg_time < 5) ab_append(ab, ctx->statusmsg, len);
}


void e_set_status_msg(e_context* ctx, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(ctx->statusmsg, sizeof(ctx->statusmsg), fmt, ap);
  ctx->statusmsg[79] = '\0';
  va_end(ap);
  ctx->statusmsg_time = time(NULL);
}


void e_get_win_size(e_context* ctx) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    e_die("ioctl");
  }

  ctx->cols = ws.ws_col;
  ctx->rows = ws.ws_row;
}


int e_rx_to_cx(e_row* row, int rx, int tab_width) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->str[cx] == '\t') cur_rx += (tab_width - 1) - (cur_rx % tab_width);
    cur_rx++;
    if (cur_rx > rx) return cx;
  }
  return cx;
}


int e_cx_to_rx(e_row* row, int cx, int tab_width) {
  int rx = 0;
  int j;

  for (j = 0; j < cx; j++) {
    if (row->str[j] == '\t') rx += (tab_width - 1) - (rx % tab_width);
    if (!isutf8cont(row->str[j])) rx++;
  }
  return rx;
}


void e_scroll(e_context* ctx) {
  ctx->rx = 0;
  if (ctx->cy < ctx->nrows) ctx->rx = e_cx_to_rx(&ctx->row[ctx->cy], ctx->cx, ctx->tab_width);

  if (ctx->cy < ctx->roff) ctx->roff = ctx->cy;
  if (ctx->cy >= ctx->roff + ctx->rows) {
    ctx->roff = ctx->cy - ctx->rows + 1;
  }
  if (ctx->rx < ctx->coff) ctx->coff = ctx->rx;
  if (ctx->rx >= ctx->coff + ctx->cols) {
    ctx->coff = ctx->rx - ctx->cols + 1;
  }
}


void e_clear_screen(e_context* ctx) {
  e_scroll(ctx);

  append_buf ab = ABUF_INIT;
  ansi_append(&ab, "?25l", 4);
  ansi_append(&ab, "H", 1);

  e_draw_rows(ctx, &ab);
  e_draw_status(ctx, &ab);
  e_draw_message(ctx, &ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "%d;%dH", ctx->cy-ctx->roff+1,
                                       ctx->rx-ctx->coff+1);
  ansi_append(&ab, buf, strlen(buf));
  ansi_append(&ab, "?25h", 4);
  e_correct_for_fullwidth(ctx, &ab);

  write_wrapped(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}


void e_move_cursor(e_context* ctx, int c) {
  int rowl = (ctx->cy >= ctx->nrows) ? 0 : ctx->row[ctx->cy].size;
  if (c == ctx->up) c = ARROW_UP;
  if (c == ctx->down) c = ARROW_DOWN;
  if (c == ctx->left) c = ARROW_LEFT;
  if (c == ctx->right) c = ARROW_RIGHT;
  switch (c) {
    case ARROW_LEFT:
      if (ctx->cx) { if (isutf8cont(ctx->row[ctx->cy].render[--ctx->cx])) ctx->cx-=2; }
      else if (ctx->cy > 0) ctx->cx = ctx->row[--ctx->cy].size;
      break;
    case ARROW_RIGHT:
      if (ctx->cx < rowl) { if (isutf8cont(ctx->row[ctx->cy].render[ctx->cx++])) ctx->cx+=2; }
      else if (ctx->cy < ctx->nrows-1) { ctx->cy++; ctx->cx = 0; }
      break;
    case ARROW_UP:
      if (ctx->cy) ctx->cy--;
      break;
    case ARROW_DOWN:
      if (ctx->cy < ctx->nrows-1) ctx->cy++;
      break;
    case PAGE_UP:
    case PAGE_DOWN:
      if (c == PAGE_UP) ctx->cy = ctx->roff;
      else ctx->cy = ctx->roff + ctx->rows - 1;
      int t = ctx->rows;
      while (t--) e_move_cursor(ctx, c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      break;
    case HOME_KEY:
      ctx->cx = 0;
      break;
    case END_KEY:
      if (ctx->cy < ctx->nrows) ctx->cx = ctx->row[ctx->cy].size;
      break;
  }
  rowl = (ctx->cy >= ctx->nrows) ? 0 : ctx->row[ctx->cy].size;
  if (ctx->cx > rowl) {
    ctx->cx = rowl;
  }
}


int e_read_key() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) e_die("read");
  }
  if (c == '\x1b') {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) return c;
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return c;

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }
    return c;
  }
  return c;
}


void meta(e_context* ctx) {
  char* c = e_prompt(ctx, "Meta:%s", NULL);

  if (!c) return;

  if (!strcmp(c, "q") || !strcmp(c, "quit")) {
    free(c);
    e_exit_prompt(ctx);
  }
  else if (!strcmp(c, "!")) {
    free(c);
    e_exit();
  }
  else if (!strcmp(c, "s") || !strcmp(c, "save")) {
    free(c);
    e_save(ctx);
    e_exit();
  }
  else if (isnum(c)) {
    int a = atoi(c);
    free(c);
    if (a > ctx->nrows) a = ctx->nrows;
    if (a <= 0) a = 1;

    ctx->cy = a-1;
  }
  else {
#ifdef WITH_LUA
    if (e_lua_meta_command(ctx, c)) {
#endif
    e_set_status_msg(ctx, "Unknown meta command.");
#ifdef WITH_LUA
    }
#endif
    free(c);
  }
}


void e_insert_newline(e_context*);


e_context* e_edit(e_context* ctx, int c) {
  int i;
  switch (c) {
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_RIGHT:
    case ARROW_LEFT:
    case PAGE_UP:
    case PAGE_DOWN:
    case HOME_KEY:
    case END_KEY:
     e_move_cursor(ctx, c);
     break;
    case ESC:
      ctx->mode = INITIAL;
      break;
    case '\r':
    case '\n': {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      e_insert_newline(new);
      return new;
    }
    case BACKSPACE:
    case CTRL('h'):
    case DEL_KEY: {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      if (c == DEL_KEY) e_move_cursor(new, ARROW_RIGHT);
      e_del_char(new);
      return new;
    }
    case CTRL('l'):
      break;
    case '\t': {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      for (i = 0; i < ctx->tab_width; i++) e_insert_char(new, ' ');
      return new;
    }
    default: {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      e_insert_char(new, c);
      return new;
    }
  }
  return ctx;
}


e_context* e_initial(e_context* ctx, int c) {
  switch (c) {
    case ':':
      meta(ctx);
      break;
    case '/':
      e_find(ctx);
      break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_RIGHT:
    case ARROW_LEFT:
    case PAGE_UP:
    case PAGE_DOWN:
    case HOME_KEY:
    case END_KEY:
     e_move_cursor(ctx, c);
     break;
    case 'e':
      ctx->mode = EDIT;
      break;
    case BACKSPACE:
    case CTRL('h'):
    case DEL_KEY: {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      if (c == DEL_KEY) e_move_cursor(new, ARROW_RIGHT);
      e_del_char(new);
      return new;
    }
    case 'x': {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      new->cx++;
      e_del_char(new);
      return new;
    }
    case ' ':
      e_save(ctx);
      break;
    case 'n': {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      e_insert_row(new, ++new->cy, (char*) "", 0);
      new->cx = 0;
      new->rx = 0;
      new->mode = EDIT;
      return new;
    }
    case 'p': {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      e_insert_row(new, new->cy, (char*) "", 0);
      new->cx = 0;
      new->rx = 0;
      new->mode = EDIT;
      return new;
    }
    case 'b':
      ctx->cx = 0;
      ctx->mode = EDIT;
      break;
    case 't':
      ctx->cx = ctx->row[ctx->cy].rsize;
      ctx->mode = EDIT;
      break;
    case 'r': {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      e_replace(new);
      return new;
    }
    case 'R': {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      e_replace_all(new);
      return new;
    }
    case 'h': {
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      e_clipboard_copy(new->row[new->cy].str);
      if (new->nrows == 1) {
        e_insert_row(new, 1, (char*) "", 0);
      }
      e_del_row(new, new->cy);
      return new;
    }
    case 'u': {
      if (ctx->history) {
        e_context* new = ctx->history;
        ctx->history = NULL;
        e_context_free(ctx);
        new->mode = INITIAL;
        return new;
      }
      e_set_status_msg(ctx, "Already at oldest change.");
      break;
    }
    case 'c':
      e_clipboard_copy(ctx->row[ctx->cy].str);
      break;
    case 'v': {
      char* str = e_clipboard_paste();
      // To prevent from crushing in linux
      if ( !str ) break;
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      int i = 0;
      while (str[i] != '\0') {
        if (str[i] == '\n' || str[i] == '\r') e_insert_newline(new);
        else e_insert_char(new, str[i]);
        i++;
      }
      free(str);
      return new;
    }
    case 'l': {
#ifdef WITH_LUA
      e_context* new = e_context_copy(ctx);
      new->history = ctx;
      char* lua_exp = e_prompt(new, "Type Lua expression: %s", NULL);
      if (!lua_exp) return new;
      char* evald   = e_lua_eval(new, lua_exp);
      if (evald) e_set_status_msg(new, "%s", evald);
      free(lua_exp);
      free(evald);
      return new;
#else
      e_set_status_msg(ctx, "e wasn't compiled with Lua support.");
      break;
#endif
    }
    default: {
      if (c == ctx->up || c == ctx->down || c == ctx->left || c == ctx->right) {
        e_move_cursor(ctx, c);
        break;
      }
#ifdef WITH_LUA
      e_context* new = e_context_copy(ctx);
      new->history = ctx;

      e_lua_key(new, c);

      return new;
#endif
    }
  }
  return ctx;
}


e_context* e_process_key(e_context* ctx) {
  int c = e_read_key();

  switch (ctx->mode) {
    case EDIT:
      ctx = e_edit(ctx, c);
      break;
    case INITIAL:
      ctx = e_initial(ctx, c);
      break;
  }

  return ctx;
}


char* e_rows_to_str(e_context* ctx, int* len) {
  int total = 0;
  int j;
  for (j = 0; j < ctx->nrows; j++) total += ctx->row[j].size + 1;
  *len = total;
  char* buf = malloc(total*sizeof(char)+1);
  char* p = buf;
  for (j = 0; j < ctx->nrows; j++) {
    memcpy(p, ctx->row[j].str, ctx->row[j].size);
    p += ctx->row[j].size;
    *p = '\n';
    p++;
  }
  buf[total] = '\0';
  return buf;
}

void e_save(e_context* ctx) {
  if (!ctx->filename) {
    ctx->filename = e_prompt(ctx, "Save as: %s", NULL);

    if (!ctx->filename) {
      e_set_status_msg(ctx, "Aborted!");
      return;
    }

    if (ctx->stxes) e_set_highlighting(ctx, ctx->stxes);
  }

  int len;
  char* buf = e_rows_to_str(ctx, &len);

  int fd = open(ctx->filename, O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    e_set_status_msg(ctx, "Cannot open file %s for writing", ctx->filename);
    goto err;
  }
  if (ftruncate(fd, len) == -1 || write(fd, buf, len) != len) {
    e_set_status_msg(ctx, "Could not write to file %s", ctx->filename);
    goto close;
  }
  ctx->dirty = 0;
  e_set_status_msg(ctx, "Wrote %d bytes to disk (file %s)", len, ctx->filename);
close:
  close(fd);
err:
  free(buf);
}


void e_update_hl(e_context* ctx, e_row* row) {
  int i = 0;
  int j;
  int k;
  int prev_sep = 1;
  int ins = 0;
  int open_pattern = row->idx ? ctx->row[row->idx-1].open_pattern : -1;
  row->hl = realloc(row->hl, row->rsize*sizeof(char));
  memset(row->hl, HL_NORMAL, row->rsize);

  if (!ctx->stx) return;

  if (open_pattern > -1) {
    pattern pat = ctx->stx->patterns[open_pattern];
    regmatch_t rem;
    if (regexec(&pat.closing, row->render, (size_t) 1, &rem, 0)) {
      memset(row->hl, pat.color, row->rsize);
      row->open_pattern = open_pattern;
      return;
    } else {
      int len = rem.rm_eo;
      for (k = 0; k < len; k++) row->hl[k] = pat.color;
      i += len;
      row->open_pattern = -1;
    }
  }

  while (i < row->rsize) {
    char c = row->render[i];
    char prev = (i>0) ? row->hl[i-1] : HL_NORMAL;

    if (ins) {
      row->hl[i] = HL_STRING;
      if (c== '\\' && row->rsize > i+1) {
        row->hl[i+1] = HL_STRING;
        i += 2;
        continue;
      }
      if (c == ins) ins = 0;
      i++;
      prev_sep = 1;
      continue;
    } else if (c == '"' || c == '\'') {
      ins = c;
      row->hl[i] = HL_STRING;
      i++;
      continue;
    }

    if ((isdigit(c) && (prev_sep || prev == HL_NUM)) ||
        (c == '.' && prev == HL_NUM)) {
      row->hl[i] = HL_NUM;
      i++;
      prev_sep = 0;
      continue;
    }

    regmatch_t rem;
    for (j = 0; j < ctx->stx->npatterns; j++) {
      pattern pat = ctx->stx->patterns[j];
      if ((!prev_sep && pat.needs_sep)) continue;
      if (!regexec(&pat.pattern, row->render+i, (size_t) 1, &rem, 0)) {
        int len = rem.rm_eo - rem.rm_so;
        if (pat.needs_sep && row->rsize > i+len && !issep(row->render[i+len])) continue;
        for (k = i; k < i+len; k++) row->hl[k] = pat.color;
        i += len;
        if (pat.multiline) {
          if(regexec(&pat.closing, row->render+i, (size_t) 1, &rem, 0)) {
            row->open_pattern = j;
          } else {
            int len = rem.rm_eo - rem.rm_so;
            for (k = i; k < i+len; k++) row->hl[k] = pat.color;
            i += len;
          }
        }
        goto cont;
      }
    }

    prev_sep = issep(c);
    i++;
    // this acts as a nested continue
cont: (void)0;
  }
}


void e_update_row(e_context* ctx, e_row* row) {
  int tabs = 0;
  int j;
  int i = 0;
  for (j = 0; j < row->size; j++) if (row->str[j] == '\t') tabs++;

  free(row->render);
  row->render = malloc((row->size+tabs*ctx->tab_width+1)*sizeof(char));

  for (j = 0; j < row->size; j++) {
    if (row->str[j] == '\t') {
      row->render[i++] = ' ';
      while (i % ctx->tab_width) row->render[i++] = ' ';
    } else {
      row->render[i++] = row->str[j];
    }
  }
  row->render[i] = '\0';
  row->rsize = i;
  int open_pattern = row->open_pattern;
  row->open_pattern = -1;
  e_update_hl(ctx, row);
  if (open_pattern != row->open_pattern) {
    for (i = row->idx+1; i < ctx->nrows; i++) {
      if (ctx->row[i].open_pattern != open_pattern) break;
      ctx->row[i].open_pattern = -1;
      e_update_hl(ctx, &ctx->row[i]);
    }
  }
}

void e_row_append(e_context* ctx, e_row* row, char* s, size_t len) {
  row->str = realloc(row->str, row->size+len+1*sizeof(char));
  memcpy(&row->str[row->size], s, len);
  row->size += len;
  row->str[row->size] = '\0';
  e_update_row(ctx, row);
}

void e_row_insert_char(e_context* ctx, e_row* row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->str = realloc(row->str, (row->size+2)*sizeof(char));
  memmove(&row->str[at+1], &row->str[at], row->size-at+1);
  row->size++;
  row->str[at] = c;
  e_update_row(ctx, row);
}


void e_row_del_char(e_context* ctx, e_row* row, int at) {
  char c = row->str[at];
  if (at < 0 || at >= row->size) return;
  memmove(&row->str[at], &row->str[at + 1], row->size - at);
  row->size--;
  e_update_row(ctx, row);
  if (isutf8cont(c)) { e_row_del_char(ctx, row, at-1); ctx->cx--; }
}


void e_free_row(e_row* row) {
  free(row->render);
  free(row->str);
  free(row->hl);
}


void e_del_row(e_context* ctx, int at) {
  int i;
  int open_pattern = ctx->row[at].open_pattern;
  if (at < 0 || at >= ctx->nrows) return;
  e_free_row(&ctx->row[at]);
  memmove(&ctx->row[at], &ctx->row[at+1], sizeof(e_row) * (ctx->nrows-at-1));
  if (open_pattern > -1) {
    for (i = at; i <= ctx->nrows-1; i++) {
      if (ctx->row[i].open_pattern != open_pattern) break;
      ctx->row[i].open_pattern = -1;
      e_update_hl(ctx, &ctx->row[i]);
    }
  }
  for (i = at; i <= ctx->nrows-1; i++) ctx->row[i].idx--;
  ctx->nrows--;
  if (ctx->cy >= ctx->nrows) ctx->cy--;
  if (ctx->cx >= ctx->row[ctx->cy].size) ctx->cx = ctx->row[ctx->cy].size;
  ctx->dirty = 1;
}


void e_insert_row(e_context* ctx, int at, char* s, size_t len) {
  int i;
  if (at < 0 || at > ctx->nrows) return;

  ctx->row = realloc(ctx->row, sizeof(e_row) * (ctx->nrows + 1));
  memmove(&ctx->row[at+1], &ctx->row[at], sizeof(e_row) * (ctx->nrows-at));
  for (i = at + 1; i <= ctx->nrows; i++) ctx->row[i].idx++;

  ctx->row[at].idx = at;
  ctx->row[at].size = len;
  ctx->row[at].str = malloc((len + 1)*sizeof(char));
  memcpy(ctx->row[at].str, s, len);
  ctx->row[at].str[len] = '\0';
  ctx->row[at].rsize = 0,
  ctx->row[at].render = NULL;
  ctx->row[at].hl = NULL;
  ctx->row[at].open_pattern = -1;
  e_update_row(ctx, &ctx->row[at]);
  ctx->nrows++;
  ctx->dirty = 1;
}


void e_append_row(e_context* ctx, char* s, size_t len) {
  e_insert_row(ctx, ctx->nrows, s, len);
}


void e_insert_char(e_context* ctx, int c) {
  if (ctx->cy == ctx->nrows) e_append_row(ctx, (char*) "", 0);

  e_row_insert_char(ctx, &ctx->row[ctx->cy], ctx->cx, c);
  ctx->cx++;
  ctx->dirty = 1;
}


void e_insert_char_at(e_context* ctx, int c, int cx, int cy) {
  if (cy == ctx->nrows) e_append_row(ctx, (char*) "", 0);

  e_row_insert_char(ctx, &ctx->row[cy], cx, c);
  ctx->dirty = 1;
}


void e_insert_newline(e_context* ctx) {
  if (ctx->cx) {
    e_row* row = &ctx->row[ctx->cy];
    e_insert_row(ctx, ctx->cy+1, &row->str[ctx->cx], row->size-ctx->cx);
    row = &ctx->row[ctx->cy];
    row->size = ctx->cx;
    row->str[row->size] = '\0';
    e_update_row(ctx, row);
  } else {
    e_insert_row(ctx, ctx->cy, (char*) "", 0);
  }
  ctx->cy++;
  ctx->cx = 0;
}


void e_del_char(e_context* ctx) {
  if (ctx->cy == ctx->nrows) return;
  if (!ctx->cx && !ctx->cy) return;

  e_row* row = &ctx->row[ctx->cy];
  if (ctx->cx > 0) {
    e_row_del_char(ctx, row, --ctx->cx);
  } else {
    ctx->cy--;
    ctx->cx = ctx->row[ctx->cy].size;
    e_row_append(ctx, &ctx->row[ctx->cy], row->str, row->size);
    e_del_row(ctx, ctx->cy+1);
  }
  ctx->dirty = 1;
}


void e_del_char_at(e_context* ctx, int cx, int cy) {
  if (ctx->cy == ctx->nrows) return;
  if (!ctx->cx && !ctx->cy) return;

  e_row* row = &ctx->row[cy];
  if (cx > 0) {
    e_row_del_char(ctx, row, cx);
  } else {
    cy--;
    cx = ctx->row[cy].size;
    e_row_append(ctx, &ctx->row[cy], row->str, row->size);
    e_del_row(ctx, cy+1);
  }
  ctx->dirty = 1;
}


void e_open(e_context* ctx, const char* filename) {
  free(ctx->filename);
  ctx->filename = strdup(filename);

  if (ctx->stxes) e_set_highlighting(ctx, ctx->stxes);

  FILE* fp = fopen(filename, "r");
  if (!fp) return;

  char* line = NULL;
  size_t linecap = 0;
  ssize_t len;
  while ((len = getline(&line, &linecap, fp)) != -1) {
    if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) len--;

    e_append_row(ctx, line, len);
  }
  free(line);
  fclose(fp);

  ctx->dirty = 0;
}


char* e_prompt(e_context* ctx, const char* prompt, e_cb callback) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize*sizeof(char));
  size_t buflen = 0;
  buf[0] = '\0';

  while (1) {
    e_set_status_msg(ctx, prompt, buf);
    e_clear_screen(ctx);
    int c = e_read_key();

    if (c == DEL_KEY || c == CTRL('h') || c == BACKSPACE) {
      if (buflen) buf[--buflen] = '\0';
    } else if (c == '\x1b' || c == CTRL('c')) {
      e_set_status_msg(ctx, "");
      if (callback) callback(ctx, buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r' && buflen) {
      e_set_status_msg(ctx, "");
      if (callback) callback(ctx, buf, c);
      return buf;
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize*sizeof(char));
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }

    if (callback) callback(ctx, buf, c);
  }
}


void e_find_cb(e_context* ctx, char* query, int key) {
  static int prev = -1;
  static signed char dir = 1;

  static int saved_line;
  static char* saved_hl = NULL;

  if (saved_hl) {
    memcpy(ctx->row[saved_line].hl, saved_hl, ctx->row[saved_line].rsize);
    free(saved_hl);
    saved_hl = NULL;
  }

  if (key == ARROW_DOWN) {
    dir = 1;
  } else if (key == ARROW_UP) {
    dir = -1;
  } else {
    prev = -1;
    dir = 1;
  }

  if (key == '\r' || key == ESC) return;

  int i;
  int cur = prev;
  regex_t re;
  regmatch_t rem;

  if (!query) return;

  if (regcomp(&re, query, REG_EXTENDED)) {
    e_set_status_msg(ctx, "Invalid regular expression.");
    return;
  }
  for (i = 0; i < ctx->nrows; i++) {
    cur += dir;
    if (cur == -1) cur = ctx->nrows - 1;
    else if (cur == ctx->nrows) cur = 0;

    e_row* row = &ctx->row[cur];
    if (!regexec(&re, row->render, (size_t) 1, &rem, 0)) {
      prev = cur;
      ctx->cy = cur;
      ctx->cx = e_rx_to_cx(row, (int) rem.rm_so, ctx->tab_width);
      ctx->roff = ctx->nrows;

      saved_line = cur;
      saved_hl = malloc(row->rsize*sizeof(char));
      memcpy(saved_hl, row->hl, row->rsize);
      memset(&row->hl[rem.rm_so], HL_MATCH, rem.rm_eo-rem.rm_so);
      break;
    }
  }
  regfree(&re);
}


void e_find(e_context* ctx) {
  int cx = ctx->cx;
  int cy = ctx->cy;
  int coff = ctx->coff;
  int roff = ctx->roff;

  char* query = e_prompt(ctx, "Search:%s", e_find_cb);

  if (query) {
    free(query);
    return;
  }
  ctx->cx = cx;
  ctx->cy = cy;
  ctx->coff = coff;
  ctx->roff = roff;
}


void e_replace_all(e_context* ctx) {
  int cx = ctx->cx;
  int cy = ctx->cy;
  int coff = ctx->coff;
  int roff = ctx->roff;

  char* query = e_prompt(ctx, "Search:%s", e_find_cb);

  if (!query) {
    ctx->cx = cx;
    ctx->cy = cy;
    ctx->coff = coff;
    ctx->roff = roff;
    return;
  }

  char* replace = e_prompt(ctx, "Replace:%s", NULL);

  if (!replace) {
    free(query);
    ctx->cx = cx;
    ctx->cy = cy;
    ctx->coff = coff;
    ctx->roff = roff;
    return;
  }

  int found = 0;
  int i;
  for (i = 0; i < ctx->nrows; i++) {
    char* res;
    e_row* row = &ctx->row[i];
    res = strsub(row->str, query, replace);
    if (res) {
      free(row->str);
      row->str = res;
      row->size = strlen(res);
      found += 1;
    }
    res = strsub(row->render, query, replace);
    if (res) {
      free(row->render);
      row->render = res;
      row->rsize = strlen(res);
    }
  }

  e_set_status_msg(ctx, "Replaced '%s' with '%s' on %d lines", query, replace, found);
  free(query);
  free(replace);
}


void e_replace(e_context* ctx) {
  int cx = ctx->cx;
  int cy = ctx->cy;
  int coff = ctx->coff;
  int roff = ctx->roff;

  char* query = e_prompt(ctx, "Search:%s", e_find_cb);

  if (!query) {
    ctx->cx = cx;
    ctx->cy = cy;
    ctx->coff = coff;
    ctx->roff = roff;
    return;
  }

  int wherex = ctx->cx;
  int wherey = ctx->cy;

  char* replace = e_prompt(ctx, "Replace:%s", NULL);

  if (!replace) {
    free(query);
    ctx->cx = cx;
    ctx->cy = cy;
    ctx->coff = coff;
    ctx->roff = roff;
    return;
  }

  int i;
  for (i = 0; i < strlen(query); i++) e_del_char_at(ctx, wherex, wherey);
  for (i = 0; i < strlen(replace); i++) {
    e_insert_char_at(ctx, (int) replace[i], wherex+i, wherey);
  }

  e_set_status_msg(ctx, "Replaced '%s' with '%s'", query, replace);
  free(query);
  free(replace);
}


void e_context_free(e_context* ctx) {
  int i;
  for (i = 0; i < ctx->nrows; i++) {
    e_free_row(&ctx->row[i]);
  }
  free(ctx->row);
  free(ctx->filename);
  if (ctx->history) e_context_free(ctx->history);
  free(ctx);
}

e_context* e_context_copy(e_context* ctx) {
  e_context* new = malloc(sizeof(e_context));
  new->orig = ctx->orig;
  new->cols = ctx->cols;
  new->rows = ctx->rows;
  new->cx = ctx->cx;
  new->rx = ctx->rx;
  new->cy = ctx->cy;
  new->mode = ctx->mode;
  new->tab_width = ctx->tab_width;
  new->up = ctx->up;
  new->down = ctx->down;
  new->left = ctx->left;
  new->right = ctx->right;
  if (ctx->filename) new->filename = strdup(ctx->filename);
  else new->filename = NULL;
  new->nrows = ctx->nrows;

  int i = 0;
  new->row = malloc(sizeof(e_row) * ctx->nrows);
  for (i = 0; i < ctx->nrows; i++) {
    e_row* row = &new->row[i];
    row->idx = ctx->row[i].idx;
    row->size = ctx->row[i].size;
    row->str = strdup(ctx->row[i].str);
    row->rsize = ctx->row[i].rsize;
    row->render = strdup(ctx->row[i].render);
    row->hl = malloc(ctx->row[i].rsize*sizeof(char));
    memcpy(row->hl, ctx->row[i].hl, ctx->row[i].rsize);
    row->open_pattern = ctx->row[i].open_pattern;
  }

  new->coff = ctx->coff;
  new->roff = ctx->roff;
  new->dirty = ctx->dirty;
  strcpy(new->statusmsg, ctx->statusmsg);
  new->statusmsg_time = ctx->statusmsg_time;

  new->history = ctx->history;


  new->stx = ctx->stx;
  new->stxes = ctx->stxes;

  return new;
}


long long e_context_size(e_context* ctx) {
  int i;
  long long size = sizeof(e_context);

  if (ctx->filename) size += strlen(ctx->filename);

  size += sizeof(e_row) * ctx->nrows;

  for (i = 0; i < ctx->nrows; i++) {
    size += strlen(ctx->row[i].render);
    size += strlen(ctx->row[i].str);
  }


  if (ctx->history) size += e_context_size(ctx->history);
  return size;
}


void e_set_highlighting(e_context* ctx, syntax** stx) {
  int i = 0;
  int j = 0;
  int k = 0;
  ctx->stx = NULL;
  ctx->stxes = stx;
  if (!ctx->filename) return;

  while (stx[i]) {
    syntax* s = stx[i];

    for (j = 0; j < s->matchlen; j++) {
      if (!regexec(&s->filematch[j], ctx->filename, 0, NULL, 0)) {
        ctx->stx = s;
        for (k = 0; k < ctx->nrows; k++) e_update_hl(ctx, &ctx->row[0]);
        return;
      }
    }
    i++;
  }
}


e_context*  e_setup() {
  e_context* ctx = malloc(sizeof(e_context));
  if (tcgetattr(STDIN_FILENO, &ctx->orig) == -1) e_die("tcgetattr");

  write_wrapped(STDOUT_FILENO, "\x1b""7\x1b[?47h", 8);
  e_get_win_size(ctx);
  enable_raw_mode(ctx);
  ctx->rx = 0;
  ctx->cx = 0;
  ctx->cy = 0;
  ctx->nrows = 0;
  ctx->row = NULL;
  ctx->filename = NULL;
  ctx->coff = 0;
  ctx->roff = 0;
  ctx->rows -= 2;
  ctx->statusmsg[0] = '\0';
  ctx->statusmsg_time = 0;
  ctx->dirty = 0;
  ctx->mode = INITIAL;
  ctx->history = NULL;
  ctx->stx = NULL;
  ctx->stxes = NULL;
  ctx->tab_width = 4;
  ctx->up = 'w';
  ctx->down = 's';
  ctx->left = 'a';
  ctx->right = 'd';
  return ctx;
}

#ifdef WITH_LUA

static void addret(lua_State *l, char* str) {
  const char *r = lua_pushfstring(l, "return %s;", str);
  if (luaL_loadbuffer(l, r, strlen(r), "=stdin") == LUA_OK) lua_remove(l, -2);
  else lua_pop(l, 2);
}


int e_lua_message(lua_State* l) {
  if (lua_gettop(l) == 1) {
    const char* x = lua_tostring(l, 1);

    lua_getglobal(l, "ctx");
    e_context* ctx = lua_touserdata(l, lua_gettop(l));

    e_set_status_msg(ctx, "lua console: %s", x);
  }

  return 0;
}


int e_lua_insert(lua_State* l) {
  if (lua_gettop(l) == 1) {
    const char* x = lua_tostring(l, 1);

    lua_getglobal(l, "ctx");
    e_context* ctx = lua_touserdata(l, lua_gettop(l));

    for (int i = 0; i < strlen(x); i++) e_insert_char(ctx, x[i]);
  }

  return 0;
}


int e_lua_insertn(lua_State* l) {
  if (lua_gettop(l) == 1) {
    const char* x = lua_tostring(l, 1);

    lua_getglobal(l, "ctx");
    e_context* ctx = lua_touserdata(l, lua_gettop(l));

    for (int i = 0; i < strlen(x); i++) e_insert_char(ctx, x[i]);
    e_insert_newline(ctx);
  }

  return 0;
}


int e_lua_delete(lua_State* l) {
  if (lua_gettop(l) == 1) {
    int x = lua_tonumber(l, 1);

    lua_getglobal(l, "ctx");
    e_context* ctx = lua_touserdata(l, lua_gettop(l));

    for (int i = 0; i < x; i++) e_del_char(ctx);
  }

  return 0;
}


int e_lua_move(lua_State* l) {
  if (lua_gettop(l) == 2) {
    int x = lua_tonumber(l, 2);
    int y = lua_tonumber(l, 1);

    lua_getglobal(l, "ctx");
    e_context* ctx = lua_touserdata(l, lua_gettop(l));

    ctx->cx = x;
    ctx->cy = y;
  }

  return 0;
}


int e_lua_get_coords(lua_State* l) {
  lua_getglobal(l, "ctx");
  e_context* ctx = lua_touserdata(l, lua_gettop(l));

  lua_pushnumber(l, ctx->cx);
  lua_pushnumber(l, ctx->cy);

  return 2;
}


int e_lua_get_bounding(lua_State* l) {
  lua_getglobal(l, "ctx");
  e_context* ctx = lua_touserdata(l, lua_gettop(l));

  lua_pushnumber(l, ctx->cols);
  lua_pushnumber(l, ctx->rows);

  return 2;
}


int e_lua_get_text(lua_State* l) {
  lua_getglobal(l, "ctx");
  e_context* ctx = lua_touserdata(l, lua_gettop(l));
  int len;

  char* str = e_rows_to_str(ctx, &len);
  lua_pushstring(l, str);

  free(str);

  return 1;
}


int e_lua_set_tab(lua_State* l) {
  if (lua_gettop(l) == 1) {
    int x = lua_tonumber(l, 1);

    lua_getglobal(l, "ctx");
    e_context* ctx = lua_touserdata(l, lua_gettop(l));

    ctx->tab_width = x;
  }

  return 0;
}


int e_lua_get_tab(lua_State* l) {
  lua_getglobal(l, "ctx");
  e_context* ctx = lua_touserdata(l, lua_gettop(l));
  lua_pushnumber(l, ctx->tab_width);

  return 1;
}


int e_lua_get_filename(lua_State* l) {
  lua_getglobal(l, "ctx");
  e_context* ctx = lua_touserdata(l, lua_gettop(l));

  lua_pushstring(l, ctx->filename);

  return 1;
}


int e_lua_open(lua_State* l) {
  if (lua_gettop(l) == 1) {
    const char* name = lua_tostring(l, 1);

    lua_getglobal(l, "ctx");
    e_context* ctx = lua_touserdata(l, lua_gettop(l));

    int i;
    for (i = 0; i < ctx->nrows; i++) {
      e_free_row(&ctx->row[i]);
    }
    free(ctx->row);
    free(ctx->filename);
    ctx->row = NULL;
    ctx->filename = NULL;
    ctx->nrows = 0;
    ctx->cx = 0;
    ctx->cy = 0;

    e_open(ctx, name);

    lua_pushlightuserdata(l, ctx);
    lua_setglobal(l, "ctx");
  }

  return 0;
}


int e_lua_prompt(lua_State* l) {
  if (lua_gettop(l) == 1) {
    const char* prompt = lua_tostring(l, 1);

    lua_getglobal(l, "ctx");
    e_context* ctx = lua_touserdata(l, lua_gettop(l));

    char* c = e_prompt(ctx, prompt, NULL);

    lua_pushstring(l, c);
  }

  return 1;
}


#define e_lua_move(dir) {\
  if (lua_gettop(l) == 1) {\
    char x = lua_tostring(l, 1)[0];\
    lua_getglobal(l, "ctx");\
    e_context* ctx = lua_touserdata(l, lua_gettop(l));\
    ctx->dir = x;\
  }\
  return 0;\
}


int e_lua_set_left(lua_State* l) {
  e_lua_move(left);
}


int e_lua_set_right(lua_State* l) {
  e_lua_move(right);
}


int e_lua_set_up(lua_State* l) {
  e_lua_move(up);
}


int e_lua_set_down(lua_State* l) {
  e_lua_move(down);
}


#undef e_lua_move


#define e_lua_get_move(dir) {\
  lua_getglobal(l, "ctx");\
  e_context* ctx = lua_touserdata(l, lua_gettop(l));\
  char x[2];\
  x[0] = ctx->dir;\
  x[1] = '\0';\
  lua_pushstring(l, x);\
  return 1;\
}


int e_lua_get_left(lua_State* l) {
  e_lua_get_move(left);
}


int e_lua_get_right(lua_State* l) {
  e_lua_get_move(right);
}


int e_lua_get_up(lua_State* l) {
  e_lua_get_move(up);
}


int e_lua_get_down(lua_State* l) {
  e_lua_get_move(down);
}


#undef e_lua_get_move


void e_initialize_lua() {
  l = luaL_newstate();
  luaL_openlibs(l);

  lua_pushcfunction(l, e_lua_message);
  lua_setglobal(l, "message");
  lua_pushcfunction(l, e_lua_insert);
  lua_setglobal(l, "insert");
  lua_pushcfunction(l, e_lua_insertn);
  lua_setglobal(l, "insertn");
  lua_pushcfunction(l, e_lua_delete);
  lua_setglobal(l, "delete");
  lua_pushcfunction(l, e_lua_move);
  lua_setglobal(l, "move");
  lua_pushcfunction(l, e_lua_get_coords);
  lua_setglobal(l, "get_coords");
  lua_pushcfunction(l, e_lua_get_bounding);
  lua_setglobal(l, "get_bounding_rect");
  lua_pushcfunction(l, e_lua_get_text);
  lua_setglobal(l, "get_text");
  lua_pushcfunction(l, e_lua_get_tab);
  lua_setglobal(l, "get_tab");
  lua_pushcfunction(l, e_lua_set_tab);
  lua_setglobal(l, "set_tab");
  lua_pushcfunction(l, e_lua_get_left);
  lua_setglobal(l, "get_left");
  lua_pushcfunction(l, e_lua_set_left);
  lua_setglobal(l, "set_left");
  lua_pushcfunction(l, e_lua_get_right);
  lua_setglobal(l, "get_right");
  lua_pushcfunction(l, e_lua_set_right);
  lua_setglobal(l, "set_right");
  lua_pushcfunction(l, e_lua_get_up);
  lua_setglobal(l, "get_up");
  lua_pushcfunction(l, e_lua_set_up);
  lua_setglobal(l, "set_up");
  lua_pushcfunction(l, e_lua_get_down);
  lua_setglobal(l, "get_down");
  lua_pushcfunction(l, e_lua_set_down);
  lua_setglobal(l, "set_down");
  lua_pushcfunction(l, e_lua_get_filename);
  lua_setglobal(l, "get_filename");
  lua_pushcfunction(l, e_lua_open);
  lua_setglobal(l, "open");
  lua_pushcfunction(l, e_lua_prompt);
  lua_setglobal(l, "prompt");
  lua_newtable(l);
  lua_setglobal(l, "keys");
  lua_newtable(l);
  lua_setglobal(l, "meta_commands");
}


char* e_lua_eval(e_context* ctx, char* str) {
  char* ret = malloc(80*sizeof(char));
  if (!l) e_initialize_lua();

  lua_pushlightuserdata(l, ctx);
  lua_setglobal(l, "ctx");

  luaL_loadbuffer(l, str, strlen(str), "=stdin");
  addret(l, str);

  if (lua_pcall(l, 0, 1, 0)) {
    snprintf(ret, 80, "lua can't execute expression: %s.", lua_tostring(l, -1));
    lua_pop(l, 1);
  } else {
    snprintf(ret, 80, "%s", lua_tostring(l, -1));
    lua_pop(l, lua_gettop(l));
  }

  ret[79] = '\0';
  return ret;
}


char* e_lua_run_file(e_context* ctx, const char* file) {
  char* ret = malloc(80*sizeof(char));
  if (!l) e_initialize_lua();

  lua_pushlightuserdata(l, ctx);
  lua_setglobal(l, "ctx");

  if (luaL_dofile(l, file)) snprintf(ret, 80, "%s", lua_tostring(l, -1));
  else ret[0] = '\0';
  lua_pop(l, lua_gettop(l));

  ret[79] = '\0';
  return ret;
}


int e_lua_get_field(const char* key) {
  lua_pushstring(l, key);
  lua_gettable(l, -2);

  if (lua_isnil(l, -1)) return 1;

  return 0;
}


int e_lua_meta_command(e_context* ctx, const char* cmd) {
  if (!l) return 1;

  lua_pushlightuserdata(l, ctx);
  lua_setglobal(l, "ctx");

  lua_getglobal(l, "meta_commands");
  if (e_lua_get_field(cmd)) return 1;

  return lua_pcall(l, 0, 1, 0);
}


int e_lua_key(e_context* ctx, int key) {
  if (!l) return 1;
  char x[2];
  x[0] = (char) key;
  x[1] = '\0';

  lua_pushlightuserdata(l, ctx);
  lua_setglobal(l, "ctx");

  lua_getglobal(l, "keys");
  if (e_lua_get_field(x)) return 1;

  return lua_pcall(l, 0, 1, 0);
}


void e_lua_free() {
  lua_close(l);
}
#endif

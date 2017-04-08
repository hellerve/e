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


void e_die(const char* s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}


void e_exit() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
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
  int h;
  int filerow;

  for (h = 0; h < ctx->rows-1; h++) {
    filerow = h + ctx->roff;
    if (h >= ctx->nrows) {
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
      ansi_append(ab, "K", 3);
      int len = ctx->row[filerow].rsize - ctx->coff;
      if (len < 0) len = 0;
      if (len > ctx->cols) len = ctx->cols;
      ab_append(ab, &ctx->row[filerow].render[ctx->coff], len);
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

  color_append(NORMAL, ab, "", 0);
  color_append(WHITE_BG, ab, " ", 1);
  char status[80];
  char rstatus[80];
  int len  = snprintf(status, sizeof(status), "%.20s - %d lines %s",
                      ctx->filename ? ctx->filename : "[No Name]", ctx->nrows,
                      ctx->dirty ? "[UNSAVED]" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", ctx->cy+1, ctx->nrows);
  color_append(BLUE, ab, status, len);
  len += 12;

  while (len < ctx->cols) {
    if (ctx->cols-len == rlen) {
      ab_append(ab, rstatus, rlen);
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


int e_rx_to_cx(e_row* row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->str[cx] == '\t') cur_rx += (E_TAB_WIDTH - 1) - (cur_rx % E_TAB_WIDTH);
    cur_rx++;
    if (cur_rx > rx) return cx;
  }
  return cx;
}


int e_cx_to_rx(e_row* row, int cx){
  int rx = 0;
  int j;

  for (j = 0; j < cx; j++) {
    if (row->str[j] == '\t') rx += (E_TAB_WIDTH - 1) - (rx % E_TAB_WIDTH);
    rx++;
  }
  return rx;
}



void e_scroll(e_context* ctx) {
  ctx->rx = 0;
  if (ctx->cy < ctx->nrows) ctx->rx = e_cx_to_rx(&ctx->row[ctx->cy], ctx->cx);

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

  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}


void e_move_cursor(e_context* ctx, int c) {
  int rowl = (ctx->cy >= ctx->nrows) ? 0 : ctx->row[ctx->cy].size;
  switch (c) {
    case 'a':
    case ARROW_LEFT:
      if (ctx->cx) ctx->cx--;
      else if (ctx->cy > 0) ctx->cx = ctx->row[--ctx->cy].size;
      break;
    case 'd':
    case ARROW_RIGHT:
      if (ctx->cx < rowl) ctx->cx++;
      else if (ctx->cy < ctx->nrows-1) { ctx->cy++; ctx->cx = 0; }
      break;
    case 'w':
    case ARROW_UP:
      if (ctx->cy) ctx->cy--;
      break;
    case 's':
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

  if (!strcmp(c, "q") || !strcmp(c, "quit")) e_exit_prompt(ctx);
  else if (!strcmp(c, "!")) e_exit();
  else if (!strcmp(c, "s") || !strcmp(c, "save")) { e_save(ctx); e_exit(); }
  else e_set_status_msg(ctx, "Unknown meta command");
}


void e_insert_newline(e_context*);


void e_edit(e_context* ctx, int c) {
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
    case '\n':
      e_insert_newline(ctx);
      break;
    case BACKSPACE:
    case CTRL('h'):
    case DEL_KEY:
      if (c == DEL_KEY) e_move_cursor(ctx, ARROW_RIGHT);
      e_del_char(ctx);
      break;
    case CTRL('l'):
      break;
    default:
      e_insert_char(ctx, c);
  }
}


void e_initial(e_context* ctx, int c) {
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
    case 'w':
    case 's':
    case 'a':
    case 'd':
     e_move_cursor(ctx, c);
     break;
    case 'e':
      ctx->mode = EDIT;
      break;
    case BACKSPACE:
    case CTRL('h'):
    case DEL_KEY:
      if (c == DEL_KEY) e_move_cursor(ctx, ARROW_RIGHT);
      e_del_char(ctx);
      break;
    case ' ':
      e_save(ctx);
      break;
  }
}


void e_process_key(e_context* ctx) {
   int c = e_read_key();

   switch (ctx->mode) {
      case EDIT:
        e_edit(ctx, c);
        break;
      case INITIAL:
        e_initial(ctx, c);
   }
}


char* e_rows_to_str(e_context* ctx, int* len) {
  int total = 0;
  int j;
  for (j = 0; j < ctx->nrows; j++) total += ctx->row[j].size + 1;
  *len = total;
  char* buf = malloc(total);
  char* p = buf;
  for (j = 0; j < ctx->nrows; j++) {
    memcpy(p, ctx->row[j].str, ctx->row[j].size);
    p += ctx->row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

void e_save(e_context* ctx) {
  if (!ctx->filename) {
    ctx->filename = e_prompt(ctx, "Save as: %s", NULL);

    if (!ctx->filename) {
      e_set_status_msg(ctx, "Aborted!");
      return;
    }
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


void e_update_row(e_row* row) {
  int tabs = 0;
  int j;
  for (j = 0; j < row->size; j++) if (row->str[j] == '\t') tabs++;

  free(row->render);
  row->render = malloc(row->size+tabs*E_TAB_WIDTH+1);

  int i = 0;
  for (j = 0; j < row->size; j++) {
    if (row->str[j] == '\t') {
      row->render[i++] = ' ';
      while (i % E_TAB_WIDTH) row->render[i++] = ' ';
    } else {
      row->render[i++] = row->str[j];
    }
  }
  row->render[i] = '\0';
  row->rsize = i;
}

void e_row_append(e_row* row, char* s, size_t len) {
  row->str = realloc(row->str, row->size+len+1);
  memcpy(&row->str[row->size], s, len);
  row->size += len;
  row->str[row->size] = '\0';
  e_update_row(row);
}

void e_row_insert_char(e_row* row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->str = realloc(row->str, row->size+2);
  memmove(&row->str[at+1], &row->str[at], row->size-at+1);
  row->size++;
  row->str[at] = c;
  e_update_row(row);
}


void e_row_del_char(e_row* row, int at) {
  if (at < 0 || at >= row->size) return;
  memmove(&row->str[at], &row->str[at + 1], row->size - at);
  row->size--;
  e_update_row(row);
}


void e_free_row(e_row* row) {
  free(row->render);
  free(row->str);
}


void e_del_row(e_context* ctx, int at) {
  if (at < 0 || at >= ctx->nrows) return;
  e_free_row(&ctx->row[at]);
  memmove(&ctx->row[at], &ctx->row[at+1], sizeof(e_row) * (ctx->nrows-at-1));
  ctx->nrows--;
  ctx->dirty = 1;
}


void e_insert_row(e_context* ctx, int at, char* s, size_t len) {
  if (at < 0 || at > ctx->nrows) return;

  ctx->row = realloc(ctx->row, sizeof(e_row) * (ctx->rows + 1));
  memmove(&ctx->row[at+1], &ctx->row[at], sizeof(e_row) * (ctx->nrows-at));

  ctx->row[at].size = len;
  ctx->row[at].str = malloc(len + 1);
  memcpy(ctx->row[at].str, s, len);
  ctx->row[at].str[len] = '\0';
  ctx->row[at].rsize = 0,
  ctx->row[at].render = NULL;
  e_update_row(ctx->row+at);
  ctx->nrows++;
  ctx->dirty = 1;
}


void e_append_row(e_context* ctx, char* s, size_t len) {
  e_insert_row(ctx, ctx->nrows, s, len);
}


void e_insert_char(e_context* ctx, int c) {
  if (ctx->cy == ctx->nrows) e_append_row(ctx, (char*) "", 0);

  e_row_insert_char(&ctx->row[ctx->cy], ctx->cx, c);
  ctx->cx++;
  ctx->dirty = 1;
}


void e_insert_newline(e_context* ctx) {
  if (ctx->cx) {
    e_row* row = &ctx->row[ctx->cy];
    e_insert_row(ctx, ctx->cy+1, &row->str[ctx->cx], row->size-ctx->cx);
    row = &ctx->row[ctx->cy];
    row->size = ctx->cx;
    row->str[row->size] = '\0';
    e_update_row(row);
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
    e_row_del_char(row, --ctx->cx);
  } else {
    ctx->cy--;
    ctx->cx = ctx->row[ctx->cy].size;
    e_row_append(&ctx->row[ctx->cy], row->str, row->size);
    e_del_row(ctx, ctx->cy+1);
  }
  ctx->dirty = 1;
}


void e_open(e_context* ctx, char* filename) {
  free(ctx->filename);
  ctx->filename = strdup(filename);

  FILE* fp = fopen(filename, "r");
  if (!fp) return;

  char* line = NULL;
  size_t linecap = 0;
  ssize_t len;
  while ((len = getline(&line, &linecap, fp)) != -1) {
    if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
    e_append_row(ctx, line, len);
  }
  free(line);
  fclose(fp);

  ctx->dirty = 0;
}


char* e_prompt(e_context* ctx, const char* prompt, e_cb callback) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);
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
        buf = realloc(buf, bufsize);
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

    e_row* row = &ctx->row[i];
    if (!regexec(&re, row->render, (size_t) 1, &rem, 0)) {
      prev = cur;
      ctx->cy = cur;
      ctx->cx = e_rx_to_cx(row, (int) rem.rm_so);
      ctx->roff = ctx->nrows;
      e_set_status_msg(ctx, "Found ", rem.rm_so, rem.rm_eo);
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


// TODO: this is where I'd wish for currying
e_context* GLOB;
void exitf() {
  disable_raw_mode(GLOB);
}


e_context*  e_setup() {
  e_context* ctx = malloc(sizeof(e_context));
  if (tcgetattr(STDIN_FILENO, &ctx->orig) == -1) e_die("tcgetattr");
  GLOB = ctx;
  atexit(exitf);

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
  ctx->rows -= 1;
  ctx->statusmsg[0] = '\0';
  ctx->statusmsg_time = 0;
  ctx->dirty = 0;
  return ctx;
}

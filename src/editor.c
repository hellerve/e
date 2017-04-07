#include "editor.h"

void disable_raw_mode(e_config* conf) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &conf->orig) == -1) e_die("tcsetattr");
}


void enable_raw_mode(e_config* conf) {
  struct termios raw;

  raw = conf->orig;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) e_die("tcsetattr");
}


void e_die(const char *s) {
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


void e_draw_rows(e_config* conf, append_buf* ab) {
  int h;
  int filerow;

  for (h = 0; h < conf->rows-1; h++) {
    filerow = h + conf->roff;
    if (h >= conf->nrows) {
      color_append(BLUE, ab, "~\x1b[K", 4);
      if (h == conf->rows / 3 && !conf->nrows) {
        char welcome[80];
        int len = snprintf(welcome, sizeof(welcome),
                           "E -- braindead editing -- version %s",
                           E_VERSION);
        if (len > conf->cols) len = conf->cols;
        int padding = (conf->cols - len) / 2;
        while (padding--) ab_append(ab, " ", 1);
        ab_append(ab, welcome, len);
      }
    } else {
      ab_append(ab, "\x1b[K", 3);
      int len = conf->row[filerow].rsize - conf->coff;
      if (len < 0) len = 0;
      if (len > conf->cols) len = conf->cols;
      ab_append(ab, &conf->row[filerow].render[conf->coff], len);
    }
    ab_append(ab, "\r\n", 2);
  }
}


void e_draw_status(e_config* conf, append_buf* ab) {
  if (conf->mode == EDIT) {
    ab_append(ab, "\x1b[43m", 5);
    color_append(BLACK, ab, "EDIT mode ", 10);
  } else if (conf->mode == INITIAL) {
    ab_append(ab, "\x1b[44m", 5);
    color_append(WHITE, ab, "INIT mode ", 10);
  }

  ab_append(ab, "\x1b[47m ", 6);
  char status[80];
  char rstatus[80];
  int len  = snprintf(status, sizeof(status), "%.20s - %d lines",
                      conf->filename ? conf->filename : "[No Name]", conf->nrows);
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", conf->cy+1, conf->nrows);
  color_append(BLUE, ab, status, len);
  len += 12;

  ab_append(ab, "\x1b[47m ", 6);
  while (len < conf->cols) {
    if (conf->cols-len == rlen) {
      color_append(BLUE, ab, rstatus, rlen);
      break;
    }
    ab_append(ab, " ", 1);
    len++;
  }
  ab_append(ab, "\r\n", 2);
}


void e_draw_message(e_config* conf, append_buf* ab) {
  ab_append(ab, "\x1b[K", 3);
  int len = strlen(conf->statusmsg);
  if (len > conf->cols) len = conf->cols;
  if (len && time(NULL) - conf->statusmsg_time < 5) ab_append(ab, conf->statusmsg, len);
}


void e_set_status_msg(e_config* conf, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(conf->statusmsg, sizeof(conf->statusmsg), fmt, ap);
  va_end(ap);
  conf->statusmsg_time = time(NULL);
}


void e_get_win_size(e_config* conf) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    e_die("ioctl");
  }

  conf->cols = ws.ws_col;
  conf->rows = ws.ws_row;
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



void e_scroll(e_config* conf) {
  conf->rx = 0;
  if (conf->cy < conf->nrows) conf->rx = e_cx_to_rx(&conf->row[conf->cy], conf->cx);

  if (conf->cy < conf->roff) conf->roff = conf->cy;
  if (conf->cy >= conf->roff + conf->rows) {
    conf->roff = conf->cy - conf->rows + 1;
  }
  if (conf->rx < conf->coff) conf->coff = conf->rx;
  if (conf->rx >= conf->coff + conf->cols) {
    conf->coff = conf->rx - conf->cols + 1;
  }
}


void e_clear_screen(e_config* conf) {
  e_scroll(conf);

  append_buf ab = ABUF_INIT;
  ab_append(&ab, "\x1b[?25l", 6);
  ab_append(&ab, "\x1b[H", 3);

  e_draw_rows(conf, &ab);
  e_draw_status(conf, &ab);
  e_draw_message(conf, &ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", conf->cy-conf->roff+1,
                                            conf->rx-conf->coff+1);
  ab_append(&ab, buf, strlen(buf));
  ab_append(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}


void e_move_cursor(e_config* conf, int c) {
  int rowl = (conf->cy >= conf->nrows) ? 0 : conf->row[conf->cy].size;
  switch (c) {
    case 'a':
    case ARROW_LEFT:
      if (conf->cx) conf->cx--;
      else if (conf->cy > 0) conf->cx = conf->row[--conf->cy].size;
      break;
    case 'd':
    case ARROW_RIGHT:
      if (conf->cx < rowl) conf->cx++;
      else if (conf->cy < conf->nrows-1) { conf->cy++; conf->cx = 0; }
      break;
    case 'w':
    case ARROW_UP:
      if (conf->cy) conf->cy--;
      break;
    case 's':
    case ARROW_DOWN:
      if (conf->cy < conf->nrows-1) conf->cy++;
      break;
    case PAGE_UP:
    case PAGE_DOWN:
      if (c == PAGE_UP) conf->cy = conf->roff;
      else conf->cy = conf->roff + conf->rows - 1;
      int t = conf->rows;
      while (t--) e_move_cursor(conf, c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      break;
    case HOME_KEY:
      conf->cx = 0;
      break;
    case END_KEY:
      if (conf->cy < conf->nrows) conf->cx = conf->row[conf->cy].size;
      break;
  }
  rowl = (conf->cy >= conf->nrows) ? 0 : conf->row[conf->cy].size;
  if (conf->cx > rowl) {
    conf->cx = rowl;
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


int e_read_meta_key(e_config* conf) {
  char c;
  write(STDOUT_FILENO, "\x1b[999B", 6);
  write(STDOUT_FILENO, "\x1b[K", 3);
  disable_raw_mode(conf);
  write(STDOUT_FILENO, "META:", 5);
  c = e_read_key();
  enable_raw_mode(conf);
  return c;
}


void meta(e_config* conf) {
  char c = e_read_meta_key(conf);

  switch (c) {
    case 'q':
      e_exit();
    default:
      e_set_status_msg(conf, "Unknown meta command");
  }
}


void e_edit(e_config* conf, int c) {
  switch (c) {
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_RIGHT:
    case ARROW_LEFT:
    case PAGE_UP:
    case PAGE_DOWN:
    case HOME_KEY:
    case END_KEY:
     e_move_cursor(conf, c);
     break;
    case ESC:
      conf->mode = INITIAL;
  }
}


void e_initial(e_config* conf, int c) {
  switch (c) {
     case ':':
       meta(conf);
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
     e_move_cursor(conf, c);
     break;
    case 'e':
      conf->mode = EDIT;
      break;
  }
}


void e_process_key(e_config* conf) {
   int c = e_read_key();

   switch (conf->mode) {
      case EDIT:
        e_edit(conf, c);
        break;
      case INITIAL:
        e_initial(conf, c);
   }
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


void e_append_row(e_config* conf, char* s, size_t len) {
  conf->row = realloc(conf->row, sizeof(e_row) * (conf->rows + 1));

  int at = conf->nrows;
  conf->row[at].size = len;
  conf->row[at].str = malloc(len + 1);
  memcpy(conf->row[at].str, s, len);
  conf->row[at].str[len] = '\0';
  conf->row[at].rsize = 0,
  conf->row[at].render = NULL;
  e_update_row(conf->row+at);
  conf->nrows++;
}


void e_open(e_config* conf, char* filename) {
  free(conf->filename);
  conf->filename = strdup(filename);

  FILE* fp = fopen(filename, "r");
  if (!fp) e_die("fopen");

  char* line = NULL;
  size_t linecap = 0;
  ssize_t len;
  while ((len = getline(&line, &linecap, fp)) != -1) {
    if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
    e_append_row(conf, line, len);
  }
  free(line);
  fclose(fp);
}


// TODO: this is where I'd wish for currying
e_config* GLOB;
void exitf() {
  disable_raw_mode(GLOB);
}


e_config*  e_setup() {
  e_config* conf = malloc(sizeof(e_config));
  if (tcgetattr(STDIN_FILENO, &conf->orig) == -1) e_die("tcgetattr");
  GLOB = conf;
  atexit(exitf);

  e_get_win_size(conf);
  enable_raw_mode(conf);
  conf->rx = 0;
  conf->cx = 0;
  conf->cy = 0;
  conf->nrows = 0;
  conf->row = NULL;
  conf->filename = NULL;
  conf->coff = 0;
  conf->roff = 0;
  conf->rows -= 1;
  conf->statusmsg[0] = '\0';
  conf->statusmsg_time = 0;
  return conf;
}

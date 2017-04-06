#include "editor.h"

e_config conf;

void disable_raw_mode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &conf.orig) == -1) e_die("tcsetattr");
}

void enable_raw_mode() {
  struct termios raw;

  raw = conf.orig;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 2;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) e_die("tcsetattr");
}

void e_die(const char *s) {
  e_clear_screen();
  perror(s);
  exit(1);
}

void e_draw_rows() {
  int h;

  for (h = 0; h < conf.rows-1; h++) {
    color_write(BLUE, STDOUT_FILENO, "~\r\n");
  }
  color_write(BLUE, STDOUT_FILENO, "~");
}

void e_get_win_size() {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    e_die("ioctl");
  }

  conf.cols = ws.ws_col;
  conf.rows = ws.ws_row;
}

void e_clear_screen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  e_draw_rows();

  write(STDOUT_FILENO, "\x1b[H", 3);
}

char e_read_key() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) e_die("read");
  }
  return c;
}

char e_read_meta_key() {
  char c;
  write(STDOUT_FILENO, "\x1b[999B", 6);
  disable_raw_mode();
  printf(":");
  fflush(stdout);
  c = e_read_key();
  enable_raw_mode();
  return c;
}

void meta() {
  char c = e_read_meta_key();

  switch (c) {
    case 'q':
      exit(0);
    default:
      puts("Unknown meta command");
  }
}

void e_process_key() {
   char c = e_read_key();

   switch (c) {
     case ':':
       meta();
       break;
   }
}

void e_setup() {
  if (tcgetattr(STDIN_FILENO, &conf.orig) == -1) e_die("tcgetattr");
  atexit(disable_raw_mode);

  e_get_win_size();
  enable_raw_mode();
}

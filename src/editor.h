#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "syntax.h"
#include "clipboard.h"

#define E_VERSION "0.2.2"

#ifndef CTRL
#define CTRL(k) ((k) & 0x1f)
#endif

typedef struct {
  int idx;
  int size;
  char* str;

  char* hl;
  short open_pattern;
} e_row;

typedef struct e_context {
  struct termios orig;
  int cols;
  int rows;

  int cx;
  int rx;
  int cy;

  int mode;
  char* filename;
  int nrows;
  e_row* row;
  int coff;
  int roff;
  char dirty;
  char statusmsg[80];
  time_t statusmsg_time;

  struct e_context* history;

  syntax* stx;
  syntax** stxes;

  unsigned short tab_width;
  char up;
  char down;
  char left;
  char right;
} e_context;

typedef void (*e_cb)(e_context*, char*, int);

enum e_key {
  ESC = 27,
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN,
  HOME_KEY,
  END_KEY,
  DEL_KEY,
};

enum e_mode {
  INITIAL,
  EDIT
};

void e_clear_screen(e_context*);
e_context* e_process_key(e_context*);
void e_die(const char*);
void e_open(e_context*, const char*);
void e_insert_char(e_context*, int);
void e_insert_char_at(e_context*, int, int, int);
void e_del_char(e_context*);
void e_del_char_at(e_context*, int, int);
void e_insert_row(e_context*, int, char*, size_t);
void e_append_row(e_context*, char*, size_t);
void e_del_row(e_context*, int);
void e_set_status_msg(e_context*, const char*, ...);
void e_save(e_context*);
char* e_prompt(e_context*, const char*, e_cb);
void e_find(e_context*);
void e_replace(e_context*);
void e_replace_all(e_context*);
long long e_context_size(e_context*);
void e_context_free(e_context*);
void e_set_highlighting(e_context*, syntax**);
e_context* e_context_copy(e_context*);
e_context* e_setup();

void enable_raw_mode(e_context*);
void disable_raw_mode(e_context*);

#ifdef WITH_LUA
#include "../vendor/lua-5.3.4/src/lua.h"
#include "../vendor/lua-5.3.4/src/lauxlib.h"
#include "../vendor/lua-5.3.4/src/lualib.h"

lua_State *l;

char* e_lua_eval(e_context*, char*);
char* e_lua_run_file(e_context*, const char*);
int   e_lua_meta_command(e_context*, const char*);
int   e_lua_key(e_context*, int);
void  e_lua_free();
#endif

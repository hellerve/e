#include <signal.h>
#include <execinfo.h>
#include "src/editor.h"

e_context* GLOB;
syntax** stx;


// This does not free.
// After a segfault we shouldn't be messing with memory
void handler(int sig) {
  disable_raw_mode(GLOB);
  void* array[10];
  size_t size;

  size = backtrace(array, 10);

  fputs("e has crashed. If you want to file a bug report, please attach the following:\n", stderr);
  fprintf(stderr, "Signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}


void exitf() {
  disable_raw_mode(GLOB);
  e_context_free(GLOB);
  if (stx) syntax_free(stx);
}


int main(int argc, char** argv) {
  stx = syntax_init((char*) STXDIR);
  GLOB = e_setup();

  signal(SIGSEGV, handler);
  signal(SIGABRT, handler);
  atexit(exitf);

  e_set_highlighting(GLOB, stx);

  if (argc > 1) {
    e_open(GLOB, argv[1]);
  }

  if (!GLOB->nrows) {
    e_append_row(GLOB, (char*) "", 0);
    GLOB->dirty = 0;
  }

  e_set_status_msg(GLOB, "HELP: :q = quit");

  while(1) {
    e_clear_screen(GLOB);
    GLOB = e_process_key(GLOB);
    // for reasons I'm not quite certain of this fixes the screen flicker.
    // TODO: investigate better technique
    msleep(20);
  }
  return 0;
}

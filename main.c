#include <signal.h>
#include <execinfo.h>
#include "src/editor.h"

e_context* GLOB;
syntax** stx;


// This does not free, because after a segfault we shouldn't be messing with memory
void handler(int sig) {
  disable_raw_mode(GLOB);
  void *array[10];
  size_t size;

  size = backtrace(array, 10);

  fputs("e has crashed. If you want to file a bug report, please attach the following:\n", stderr);
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(0);
}


void exitf() {
  disable_raw_mode(GLOB);
  e_context_free(GLOB);
  syntax_free(stx);
}


int main(int argc, char** argv) {
  stx = syntax_init((char*) STXDIR);
  signal(SIGSEGV, handler);
  signal(SIGABRT, handler);
  GLOB = e_setup();
  atexit(exitf);

  e_set_highlighting(GLOB, stx);

  if (argc > 1) {
    e_open(GLOB, argv[1]);
  }

  if (!GLOB->nrows) {
    e_append_row(GLOB, (char*) "", 0);
  }

  e_set_status_msg(GLOB, "HELP: :q = quit");

  while(1) {
    e_clear_screen(GLOB);
    GLOB = e_process_key(GLOB);
  }
  return 0;
}

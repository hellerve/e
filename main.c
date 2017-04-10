#include "src/editor.h"

e_context* GLOB;


void exitf() {
  disable_raw_mode(GLOB);
  e_context_free(GLOB);
}


int main(int argc, char** argv) {
  GLOB = e_setup();
  atexit(exitf);

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

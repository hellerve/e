#include "src/editor.h"

int main(int argc, char** argv) {
  e_context* e = e_setup();

  if (argc > 1) {
    e_open(e, argv[1]);
  }

  e_set_status_msg(e, "HELP: :q = quit");

  while(1) {
    e_clear_screen(e);
    e_process_key(e);
  }
  return 0;
}

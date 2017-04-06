#include "editor.h"

int main() {
  e_setup();

  while(1) {
    e_clear_screen();
    e_process_key();
  }
  return 0;
}

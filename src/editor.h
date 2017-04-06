#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "colors.h"

typedef struct {
  struct termios orig;
  int cols;
  int rows;
} e_config;

void e_clear_screen();
void e_process_key();
void e_die(const char *s);
void e_setup();

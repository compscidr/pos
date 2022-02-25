#include "fs/fat.h"
#include "screen.h"

// todo: abstract this so that it reads via the device driver for the particular
// block storage device
void ls(char * path) {
  print_string("/\n");
}
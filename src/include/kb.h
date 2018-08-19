#ifndef KB_HEADER
#define KB_HEADER

#include "common.h"

#define KB_BUFFER_SIZE 1024

void kb_init(void);
void kb_handler(struct regs *r);
char * kb_gets(char * str);
void reboot(void);

#endif

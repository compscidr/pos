#ifndef ISRS_HEADER
#define ISRS_HEADER

#include "common.h"

void interrupt_init(void);
void irq_install_handler(int irq, void (*handler)(struct regs *r));
void irq_wait(int irq);

#endif

#include "screen.h"
#include "cli.h"

int main(void) {
    screen_init();
    screen_clear();
    print_string("I'm in the kernel now!!");

//  /* Global Descriptor Table - Sets up Rings, Flat memory segments
//   * (ie, 4GB available for each ring) */
//  gdt_init();
//
//  /* Set up pointer to screen memory, init the cursors */
//  screen_init();
//  screen_clear();
//
//  ipv4_init();
//  udp_init();
//
//  /* Create the table of exception and interrupt functions */
//  print_string("Setting Up Interrupts...");
//  interrupt_init();
//  print_status(1);
//
//  /* Sets up the thread structures so we can do context switches */
//  print_string("Initializing Threads & System Timer...");
//  thread_init();
//
//  /* Starts the system clock */
//  timer_init();
//  print_status(1);
//
//  /* Enable interrupts */
//  asm volatile ("sti");
//
//  /* Scan the PCI bus for devices we recognize and init the driver for each device */
//  print_string("Scanning PCI Bus for devices...\n");
//  pci_init();
//  print_string("Scanning complete.\n");
//
//  /* Detect and initialize any floppy drives */
//  fdd_initialize();
//
//  /* start the command line interface (CLI) */
//  create_task(cli_main);
//  kb_init();

  while (1) {
    __asm__("hlt");
  }
}

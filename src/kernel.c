#include "gdt.h"
#include "idt.h"
#include "task.h"
#include "timer.h"
#include "screen.h"
#include "cli.h"
#include "kb.h"
#include "pci.h"

int main(void) {
  
  /* Global Descriptor Table - Sets up Rings, Flat memory segments 
   * (ie, 4GB available for each ring) */
  gdt_init();
	
  /* Set up pointer to screen memory, init the cursors */
  screen_init();
  screen_clear();
  
  /* Create the table of exception and interrupt functions */
  print_string("Setting Up Interrupts...");
  interrupt_init();		
  print_status(1);
  
  /* Sets up the thread structures so we can do context switches */
  print_string("Intializing Threads & System Timer...");
  thread_init();			
  
  /* Starts the system clock */
  timer_init();
  print_status(1);
  
  /* Enable interrupts */
  asm volatile ("sti");	
  
  /* Scan the PCI bus for devices we recognize and init the driver for each device */
  print_string("Scanning PCI Bus for devices...\n");
  pci_init();
  print_string("Scanning complete.\n");
  
  /* start the command line interface (CLI) */
  create_task(cli_main);	
  kb_init();
  
  while (1) {
    __asm__("hlt");
  }
}

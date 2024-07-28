#include "common.h"
#include "screen.h"
#include "idt.h"

void idt_init(void);
void isrs_init(void);
void irq_init(void);
void irq_remap(void);
void fault_handler(struct regs *r);
void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags);

/* Define an IDT entry */
struct idt_entry
{
  unsigned short base_lo;
  unsigned short sel;		/* Our kernel segment goes here */
  unsigned char always0;	/* This is always set to zero */
  unsigned char flags;		/* http://www.osdever.net/bkerndev/Docs/idt.htm */
  unsigned short base_hi;
} __attribute__((packed));

/* Define pointer to the entire IDT */
struct idt_ptr
{
  unsigned short limit;
  unsigned int base;
} __attribute__((packed));

extern void idt_load();		/* defined in util.s */
extern void isr0();			/* handler prototypes for all ISRs */
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void irq0();		/* handler prototypes for IRQ */
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

struct idt_entry idt[256];	/* 256 idts */
struct idt_ptr idtp;		/* pointer to idt */
static volatile unsigned char irq_received[16] = {0};

/* Array of function pointers, used to handle custom IRQ handlers for given IRQ */
void *irq_routines[16] =
{
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0
};

/*
 * Initializes the interrupt system
 * - install interrupt descriptor table (IDT)
 * - install interrupt service routines (ISRs)
 * - install interrupts (IRQs)
 */ 
void interrupt_init(void)
{
	idt_init();	
	isrs_init();
	irq_init();
	
	irq_install_handler(0, irq0);
	
	/* Install the IDT */
	idt_load();
}

/* Set an entry in the IDT */
void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags)
{
	/* The Interrupt Service Routine (ISR) base address */
	idt[num].base_lo = (base & 0xFFFF);
	idt[num].base_hi = (base >> 16) & 0xFFFF;
	
	/* The segment / selector that this IDT entry will use */
	idt[num].sel = sel;
	idt[num].always0 = 0;
	idt[num].flags = flags;
}

/*
 * Initialize the interrupt descriptor table (IDT)
 * - set idt entries to zero
 * - link pointer with table
 * - load the table with the lidt instruction (external via util.s)
 */
void idt_init(void)
{
	/* Sets IDT pointer up */
	idtp.limit = (sizeof (struct idt_entry) * 256) - 1;
	idtp.base = (unsigned int)&idt;
	
	/* Clear out entire IDT (set to zero) */
	memset(&idt, 0, sizeof(struct idt_entry) * 256);	
}

/*
 * Initialize the interrupt service routines
 * - Install default exception routines to entries in IDT 
 */
void isrs_init(void)
{
	idt_set_gate(0, (unsigned)isr0, 0x08, 0x8E);
	idt_set_gate(1, (unsigned)isr1, 0x08, 0x8E);
	idt_set_gate(2, (unsigned)isr2, 0x08, 0x8E);
	idt_set_gate(3, (unsigned)isr3, 0x08, 0x8E);
	idt_set_gate(4, (unsigned)isr4, 0x08, 0x8E);
	idt_set_gate(5, (unsigned)isr5, 0x08, 0x8E);
	idt_set_gate(6, (unsigned)isr6, 0x08, 0x8E);
	idt_set_gate(7, (unsigned)isr7, 0x08, 0x8E);
	idt_set_gate(8, (unsigned)isr8, 0x08, 0x8E);
	idt_set_gate(9, (unsigned)isr9, 0x08, 0x8E);
	idt_set_gate(10, (unsigned)isr10, 0x08, 0x8E);
	idt_set_gate(11, (unsigned)isr11, 0x08, 0x8E);
	idt_set_gate(12, (unsigned)isr12, 0x08, 0x8E);
	idt_set_gate(13, (unsigned)isr13, 0x08, 0x8E);
	idt_set_gate(14, (unsigned)isr14, 0x08, 0x8E);
	idt_set_gate(15, (unsigned)isr15, 0x08, 0x8E);
	idt_set_gate(16, (unsigned)isr16, 0x08, 0x8E);
	idt_set_gate(17, (unsigned)isr17, 0x08, 0x8E);
	idt_set_gate(18, (unsigned)isr18, 0x08, 0x8E);
	idt_set_gate(19, (unsigned)isr19, 0x08, 0x8E);
	idt_set_gate(20, (unsigned)isr20, 0x08, 0x8E);
	idt_set_gate(21, (unsigned)isr21, 0x08, 0x8E);
	idt_set_gate(22, (unsigned)isr22, 0x08, 0x8E);
	idt_set_gate(23, (unsigned)isr23, 0x08, 0x8E);
	idt_set_gate(24, (unsigned)isr24, 0x08, 0x8E);
	idt_set_gate(25, (unsigned)isr25, 0x08, 0x8E);
	idt_set_gate(26, (unsigned)isr26, 0x08, 0x8E);
	idt_set_gate(27, (unsigned)isr27, 0x08, 0x8E);
	idt_set_gate(28, (unsigned)isr28, 0x08, 0x8E);
	idt_set_gate(29, (unsigned)isr29, 0x08, 0x8E);
	idt_set_gate(30, (unsigned)isr30, 0x08, 0x8E);
	idt_set_gate(31, (unsigned)isr31, 0x08, 0x8E);
}

/* Initialize the IRQ routines
 * - Remap the IRQs
 * - Install default ISRs to entries in IDT 
 */
void irq_init(void)
{
	irq_remap();
	idt_set_gate(32, (unsigned)irq0, 0x08, 0x8E);
	idt_set_gate(33, (unsigned)irq1, 0x08, 0x8E);
	idt_set_gate(34, (unsigned)irq2, 0x08, 0x8E);
	idt_set_gate(35, (unsigned)irq3, 0x08, 0x8E);
	idt_set_gate(36, (unsigned)irq4, 0x08, 0x8E);
	idt_set_gate(37, (unsigned)irq5, 0x08, 0x8E);
	idt_set_gate(38, (unsigned)irq6, 0x08, 0x8E);
	idt_set_gate(39, (unsigned)irq7, 0x08, 0x8E);
	idt_set_gate(40, (unsigned)irq8, 0x08, 0x8E);
	idt_set_gate(41, (unsigned)irq9, 0x08, 0x8E);
	idt_set_gate(42, (unsigned)irq10, 0x08, 0x8E);
	idt_set_gate(43, (unsigned)irq11, 0x08, 0x8E);
	idt_set_gate(44, (unsigned)irq12, 0x08, 0x8E);
	idt_set_gate(45, (unsigned)irq13, 0x08, 0x8E);
	idt_set_gate(46, (unsigned)irq14, 0x08, 0x8E);
	idt_set_gate(47, (unsigned)irq15, 0x08, 0x8E);
}

/* 
 * MAP IRQ 0 - 15 to IDT 32 - 47 rather than 8 - 15 
 */
void irq_remap(void)
{
	outportb(0x20, 0x11);
	outportb(0xA0, 0x11);
	outportb(0x21, 0x20);
	outportb(0xA1, 0x28);
	outportb(0x21, 0x04);
	outportb(0xA1, 0x02);
	outportb(0x21, 0x01);
	outportb(0xA1, 0x01);
	outportb(0x21, 0x0);
	outportb(0xA1, 0x0);
}

/*
 * Common IRQ handling routine
 * - Get the interrupt # from the regs that get pushed by interrupt
 * - Look up interrupt # in IDT table
 * - If handler exists, call it
 * - Notify master, and if necessary - notify slave
 */
void irq_handler(struct regs *r)
{
	/* Blank function pointer */
	void (*handler)(struct regs *r);
	handler = irq_routines[r->int_no - 32];
	irq_received[r->int_no - 32] = 1;

    char temp[33] = {0};
    itoa(r->int_no - 32, temp, 10);
//    print_string("IRQ ");
//    print_string(temp);
//    print_string(" received\n");

	/* Execute the interrupt handler routine */
	if(handler)
		handler(r);

    // let the wait for IRQ function clear this after it's done
    // irq_received[r->int_no - 32] = 0;

	/* If IDT > 40 (ie IRQ8-IRQ15), send EOI to slave */
	if(r->int_no >= 40)
	outportb(0xA0, 0x20);

	/* Always send EOI to master */
	outportb(0x20, 0x20);
}

/* 
 * Install a custom IRQ handler for the given IRQ 
 */
void irq_install_handler(int irq, void (*handler)(struct regs *r))
{
	irq_routines[irq] = handler;
}

/* String array of exception messages */
char * const exception_messages[] =
{
	"Division by Zero",
	"Debug",
	"Non Maskable Interrupt",
	"Breakpoint",
	"Into Detected Overflow",
	"Out of Bounds",
	"Invalid Opcode",
	"No Coprocessor",

	"Double Fault",
	"Coprocessors Segment Overrun",
	"Bad TSS",
	"Segment Not Present",
	"Stack Fault",
	"General Protection Fault",
	"Page Fault",
	"Unknown Interrupt",

	"Coprocessor Fault",
	"Alignment Check",
	"Machine Check",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",

	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
};

/*
 * Common fault handler routine
 * - Display and error message regarding the exception
 * - TODO: add register dump to provide more information
 */
void fault_handler(struct regs *r)
{
	/* Check if fault between 0 and 31 */
	if(r->int_no < 32)
	{
		print_string(exception_messages[r->int_no]);
		print_string(" Exception. System Halted!\n");
		for(;;);
	}
	else
	{
		print_string("Serious Screw Up, stopping exec\n");
		for(;;);
	}
}

void irq_wait(int irq) {
//  print_string("Waiting for IRQ ");
//  char temp[33] = {0};
//  itoa(irq, temp, 10);
//  print_string(temp);
//  print_string("\n");
  // note: only a single thread can wait for a particular irq at a time for now
  while (!irq_received[irq]) {
    __asm__("hlt");
  }
  irq_received[irq] = 0;
}
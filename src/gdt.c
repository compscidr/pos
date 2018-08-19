#include "common.h"

/* Defines a GDT entry. We say packed, because it prevents the
*  compiler from doing things that it thinks is best: Prevent
*  compiler "optimization" by packing */
struct gdt_entry
{
  unsigned short limit_low;
  unsigned short base_low;
  unsigned char base_middle;
  unsigned char access;
  unsigned char granularity;
  unsigned char base_high;
} __attribute__((packed));

/* Special pointer which includes the limit: The max bytes
*  taken up by the GDT, minus 1. Again, this NEEDS to be packed */
struct gdt_ptr
{
  unsigned short limit;
  unsigned int base;
} __attribute__((packed));

struct tss_entry {
  unsigned short link;
  unsigned short link_h;

  unsigned long esp0;					// The stack pointer to load when we change to kernel mode.
  unsigned short ss0;					// The stack segment to load when we change to kernel mode.
  unsigned short ss0_h;

  unsigned long esp1;
  unsigned short ss1;
  unsigned short ss1_h;

  unsigned long esp2;
  unsigned short ss2;
  unsigned short ss2_h;

  unsigned long cr3;
  unsigned long eip;
  unsigned long eflags;
  
  unsigned long eax;
  unsigned long ecx;
  unsigned long edx;
  unsigned long ebx;

  unsigned long esp;
  unsigned long ebp;

  unsigned long esi;
  unsigned long edi;

  unsigned short es;
  unsigned short es_h;

  unsigned short cs;
  unsigned short cs_h;

  unsigned short ss;
  unsigned short ss_h;

  unsigned short ds;
  unsigned short ds_h;

  unsigned short fs;
  unsigned short fs_h;
  
  unsigned short gs;
  unsigned short gs_h;

  unsigned short ldt;
  unsigned short ldt_h;

  unsigned short trap;
  unsigned short iomap;
} __attribute__((packed));

/* Our GDT, with 3 entries, and finally our special GDT pointer */
/* Update 4th entry for TSS, and tss_table pointer */
struct gdt_entry gdt[4];
struct gdt_ptr gp;
struct tss_entry tss;

/* This will be a function in start.asm. We use this to properly
*  reload the new segment registers */
extern void gdt_flush();

/* Setup a descriptor in the Global Descriptor Table */
void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran)
{
  /* Setup the descriptor base address */
  gdt[num].base_low = (base & 0xFFFF);
  gdt[num].base_middle = (base >> 16) & 0xFF;
  gdt[num].base_high = (base >> 24) & 0xFF;

  /* Setup the descriptor limits */
  gdt[num].limit_low = (limit & 0xFFFF);
  gdt[num].granularity = ((limit >> 16) & 0x0F);

  /* Finally, set up the granularity and access flags */
  gdt[num].granularity |= (gran & 0xF0);
  gdt[num].access = access;
}

/* Should be called by main. This will setup the special GDT
*  pointer, set up the first 3 entries in our GDT, and then
*  finally call gdt_flush() in our assembler file in order
*  to tell the processor where the new GDT is and update the
*  new segment registers */
void gdt_init(void)
{
  /* Setup the GDT pointer and limit */
  gp.limit = (sizeof(struct gdt_entry) * 5) - 1;
  gp.base = (unsigned int)&gdt;

  /* Our NULL descriptor */
  gdt_set_gate(0, 0, 0, 0, 0);

  /* The second entry is our Code Segment. The base address
   * is 0, the limit is 4GBytes, it uses 4KByte granularity,
   * uses 32-bit opcodes, and is a Code Segment descriptor.
   * Setup the GDT pointer and limit */
  gp.limit = (sizeof(struct gdt_entry) * 5) - 1;
  gp.base = (unsigned int)&gdt;

  /* Our NULL descriptor */
  gdt_set_gate(0, 0, 0, 0, 0);

  /* The second entry is our Code Segment. The base address
   * is 0, the limit is 4GBytes, it uses 4KByte granularity,
   * uses 32-bit opcodes, and is a Code Segment descriptor.
   * Please check the table above in the tutorial in order
   * to see exactly what each value means */
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

  /* The third entry is our Data Segment. It's EXACTLY the
   *  same as our code segment, but the descriptor type in
   *  this entry's access byte says it's a Data Segment */
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

  /* The fourth entry is the Task State Segement */
  /* may need to switch 0x89 (access) and 0x5f (granularity) */
  gdt_set_gate(3, (unsigned long int)&tss, sizeof(struct tss_entry), 0x89, 0x00);
		
  memset(&tss, 0, sizeof(struct tss_entry));
  tss.ss0 = 0x10;										//set stack to data segment
  tss.esp0 = 0x0;
			
  /* Flush out the old GDT and install the new changes! */
  gdt_flush();
		   
  asm volatile ("ltr %%ax;"::"a"(3 << 3));		//gate is 3 but we set the last two bits for RPL = 3
}

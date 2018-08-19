#include "common.h"
#include "screen.h"
#include "idt.h"
#include "kb.h"

#define KB_META_ALT		0x0200
#define KB_META_CTRL	0x0400
#define KB_META_SHIFT	0x0800

#define KB_ALT			0x38
#define KB_CTRL			0x1D
#define KB_DEL			0x53

char kb_buffer[KB_BUFFER_SIZE];
int input_ready;
int buffer_position;
void reboot(void);	

void kb_init(void)
{
  irq_install_handler(1, kb_handler);
  input_ready = -1;
  buffer_position = 0;
}

char kbcdn[128] =
{
  0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '0', '-', '=' , '\b', '\t', 'q', 'w', 'e', 'r', 't',
  'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a',
  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'',
  '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm',
  ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+',
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void kb_handler(struct regs *r)
{
  int scancode;
  static unsigned status;

  scancode = inportb(0x60);
  if(scancode & 0x80)
  {
    
  }
  else
  {
    if(kbcdn[scancode] == '\b')
    {
      if(buffer_position > 0)
      {
        print_char(kbcdn[scancode]);
        buffer_position--;
      }
    }
    else
    {
      kb_buffer[buffer_position++] = kbcdn[scancode];
      print_char(kbcdn[scancode]);
      if(kbcdn[scancode] == '\n')
      {
        kb_buffer[buffer_position-1] = '\0';
        buffer_position = 0;
        input_ready = 1;
      }
    }//end else
  }

  if(scancode == KB_ALT)
  {
    status |= KB_META_ALT;
    return;
  }
    
  if(scancode == KB_CTRL)
  {
    status |= KB_META_CTRL;
    return;
  }
    
  if((status & KB_META_ALT) && (status & KB_META_CTRL) && (scancode == KB_DEL))
  {
    reboot();
    return;
  }
    
  r = r;
}

/*
 * Restarts the computer
 * Use the KB controller to pulse the CPU reset pin
 */
#define bit(n) (1<<(n))
#define check_flag(flags,n) ((flags) & bit(n))
void reboot(void)
{
  unsigned char temp;
  __asm__ __volatile__ ("cli");	/* disable interrupts */
  do
  {
    temp = inportb(0x64);		/* empty user data */
    if(check_flag(temp, 0) != 0)
      inportb(0x60); 			/* empty kb data */
  } while (check_flag(temp, 1) != 0);
  outportb(0x64, 0xfe); 			/* pulse CPU reset line */
}

/*
 * blocks until the user presses enter
 */
char * kb_gets(char * str)
{
  int length;
  while(input_ready == -1) {__asm__("hlt");}
  input_ready = -1;
  length = strlen(kb_buffer);
  memcpy(str, kb_buffer, length+1);
  return str;
}

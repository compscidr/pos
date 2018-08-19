#include "common.h"
#include "screen.h"

unsigned short * textmemptr;		/* location in mem where next char goes */
int cursor_x, cursor_y;				/* x,y co-ordinates where the next char goes */
int attribute;						/* foreground & background color */

/*
 * Sets the text pointer to the start of VGA memory, sets cursor to 
 * (0,0) and sets the default color to white on black
 */
void screen_init(void)
{
  textmemptr = (unsigned short *)0xB8000;
  cursor_x = 0;
  cursor_y = 0;
  attribute = 0x07;
}

/*
 * Updates the position of the cursor - should be called after character or
 * a string has been printed to the screen.
 */
void screen_move(void)
{
  unsigned int base_address = 0x3D4;
  unsigned int index = cursor_y * 80 + cursor_x;
  /* High byte */
  outportb(base_address, 14);
  outportb(base_address+1, (index >> 8));
  /* Low byte */
  outportb(base_address, 15);
  outportb(base_address+1, index);
}

/*
 * Blanks the entire screen
 */
void screen_clear(void)
{
  int c;
  for(c=0;c<80*25;c++)
    print_char(' ');
  cursor_x = 0;
  cursor_y = 0;
  screen_move();
}

/*
 * Scrolls the screen if necessary
 */
void screen_scroll(void)
{
  unsigned short temp[80];	/* Used to temporarily hold one line */
  unsigned int spos, dpos, c;
    
  if(cursor_y >= 25)
  {		
    /* blank out the first line */
    cursor_x = 0;
    cursor_y = 0;
    for(c=0; c< 80; c++)
      print_char(' ');
    for(c=0; c< 80; c++)
      temp[c] = textmemptr[c];
      
    /* bump everything else up one line */	
    dpos = 0;
    spos = 80;
      
    while(spos < 80*25)
    {
      textmemptr[dpos] = textmemptr[spos];
      dpos++;
      spos++;
    }
    for(c=0;c<80;c++)
      textmemptr[c+(80*24)] = temp[c];
    cursor_x = 0;			/* restore the cursor to the last line */
    cursor_y = 24;
  }
}

/*
 * Writes a single character to the screen, or moves the cursor around
 * depending on the character (ex: backspace, tab, return etc.)
 */
void print_char(unsigned char c)
{
  unsigned short *where;
  unsigned int attrib = attribute << 8;
    
  switch(c)
  {
    case '\b':
      cursor_x--;
      print_char(' ');
      cursor_x--;
    break;
    case '\t':
      cursor_x = (cursor_x + 8) & ~(8-1);
    break;
    case '\r':
      cursor_x = 0;
    break;
    case '\n':
      print_char(' ');			//updated may2012 - blank out the rest of the line instead of just moving it
      while(cursor_x != 0)
        print_char(' ');
    break;
    default:
    if(c >= ' ')
    {
      where = textmemptr + (cursor_y * 80 + cursor_x);
      *where = c | attrib;
      cursor_x++;
    }
  }
    
  /* Check if cursor is at edge of screen */
  if(cursor_x >= 80)
  {
    cursor_x = 0;
    cursor_y++;
  }
  if(cursor_x < 0)
  {
    cursor_x = 79 + cursor_x;
    cursor_y--;
  }
    
  /* Scoll and move the cursor if necessary */
  screen_scroll();
  screen_move();
}

/*
 * Prints an address in base 10 to the screen
 */
void print_address(unsigned long addr)
{
  //unsigned int i = 0;
  char temp[33] = {0};
  print_string("0x");
  print_string(itoa(addr,temp,16));
  /*
  unsigned char * ptr = (unsigned char *)&addr;
  for(i = 0; i < sizeof(addr); i++)
  {
    int res = 0;
    res += ptr[i];
    print_string(itoa(res,temp,16));
  }*/
}

/*
 * Prints a null-terminated string to the screen
 */
void print_string(char * string)
{
  while(*string != '\0')
    print_char(*string++);
}

/* 
 * Puts a string onto the screen at a particular location - useful for 
 * centering, placing text at bottom of screen for status bar etc.) 
 */
void print_string_at(char * text, int x, int y)
{
  int old_cursor_x = cursor_x;
  int old_cursor_y = cursor_y;

  cursor_x = x;
  cursor_y = y;
  print_string(text);

  cursor_x = old_cursor_x;
  cursor_y = old_cursor_y;
    
  screen_move();
}

/* 
 * Prints a string onto the screen at a particular x-coordinate. Useful
 * when we want to align things, assumes the user isnt stupid and prints
 * things off the edge of the screen 
 */
void print_string_atx(char * text, int x)
{
  cursor_x = x;
  print_string(text);
}

/* 
 * Sets the forecolor and backcolor that we will use 
 */
void settextcolor(unsigned char forecolor, unsigned char backcolor)
{
  attribute = (backcolor << 4) | (forecolor & 0x0F);
}

/*
 * puts status text, right aligned, in brackets. Green and OK if status == 1, Red and FAIL if status != 1
 */
void print_status(unsigned char status)
{
  if(status == 1)
  {
    print_string_atx("[", 76);
    settextcolor(VGAGREEN, VGABLACK);
    print_string("OK");
    settextcolor(VGAWHITE, VGABLACK);
    print_string("]");
  }
  else
  {
    print_string_atx("[",74);
    settextcolor(VGARED, VGABLACK);
    print_string("FAIL");
    settextcolor(VGAWHITE, VGABLACK);
    print_string("]");
  }
}

/*
 * assumes there is extra space in the string
 */
char * pad (char * input, int length)
{
  int l = strlen(input);
  if(length > l)
  {
    int diff = length - l;
    int i;
    for(i = length; i >= 0; i--)
    {
      if(i-diff >= 0)
        input[i] = input[i-diff];
      else
        input[i] = '0';
    }
  }
  return input;
}

/*
 * Used in the hd function - maybe can generalize to handle ints rather
 * than strings that have already been converted
 */
void print_num(char * str)
{
  while((*str != 0) && (*(str+1) != 0))
  {
    if(*str == '0' && *(str+1) == '0')
      settextcolor(VGAGREY, VGABLACK);
    else
      settextcolor(VGAWHITE, VGABLACK);
    print_char(*str++);
    print_char(*str++);
  }
  settextcolor(VGAWHITE, VGABLACK);
}

/*
 * Outputs a hex dump from start_location to end_location
 */
void hd(unsigned long int start_location, unsigned long int end_location)
{
  if(start_location < end_location)
  {
    while(start_location < end_location)
    {
      char temp[33];
      print_address(start_location);
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)start_location, temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 1), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 2), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 3), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 4), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 5), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 6), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 7), temp, 16), 2));
      print_string("  ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 8), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 9), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 10), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 11), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 12), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 13), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 14), temp, 16), 2));
      print_string(" ");
      print_num(pad(itoa(*(unsigned char *)(start_location + 15), temp, 16), 2));
      print_string("\n");
      start_location += 16;
    }
  }
}

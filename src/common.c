#include "common.h"

/*
 * Reads a byte from a port
 */
unsigned char inportb(unsigned short port)
{
  unsigned char rv;
  __asm__ __volatile__ ("in %%dx,%%al" : "=a" (rv) : "d" (port));
  return rv;
}

/*
 * Reads a byte from a port and waits until it completes.
 */
unsigned char inportb_p(unsigned short port)
{
  unsigned char rv;
  __asm__ __volatile__ ("inb %w1,%0\noutb %%al,$0x80":"=a" (rv):"Nd" (port));
  return rv;
}

/*
 * Reads a word (2 bytes) from a port
 */
unsigned short inportw(unsigned short port)
{
  unsigned short rv;
  __asm__ __volatile__ ("inw %%dx, %%ax" : "=a" (rv) : "d" (port));
  return rv;
}

/*
 * Reads a word (2 bytes) from a port and waits until it completes
 */
unsigned short inportw_p(unsigned short port)
{
  unsigned short rv;
  __asm__ __volatile__ ("inw %w1,%0\noutb %%al,$0x80":"=a" (rv):"Nd" (port));
  return rv;
}

/*
 * Reads a long (4 bytes) from a port
 */
unsigned long int inportl(unsigned short port)
{
  unsigned long int rv;
  __asm__ __volatile__ ("in %%dx,%%eax" : "=a" (rv) : "d"(port));
  return rv;
}

/*
 * Reads a long (4 bytes) from a port and waits until it completes
 */
unsigned long int inportl_p(unsigned short port)
{
  unsigned long int rv;
  __asm__ __volatile__ ("inl %w1,%0\noutb %%al,$0x80":"=a" (rv):"Nd" (port));
  return rv;
}

/*
 * Reads count bytes into the address from given the port.
 */
void inportsb(unsigned short port, void * addr, unsigned long int count) {
  __asm__ __volatile__ ("cld ; rep ; insb":"=D" (addr), "=c" (count)
                       :"d" (port), "0" (addr), "1" (count));
}

/*
 * Reads count words (x 2 bytes) into the address from the given port.
 */
void inportsw(unsigned short port, void * addr, unsigned long int count) {
  __asm__ __volatile__ ("cld ; rep ; insw":"=D" (addr), "=c" (count)
                       :"d" (port), "0" (addr), "1" (count));
}

/*
 * Reads count longs (x 4 bytes) into the address from the given port.
 */
void inportsl(unsigned short port, void * addr, unsigned long int count) {
  __asm__ __volatile__ ("cld ; rep ; insl":"=D" (addr), "=c" (count)
                       :"d" (port), "0" (addr), "1" (count));
}

/*
 * Writes a bytes to a port
 */
void outportb(unsigned short port, unsigned char data)
{
  __asm__ __volatile__ ("outb %%al,%%dx" : : "a" (data), "d" (port));
}

/*
 * Writes a bytes to a port and waits until it completes
 */
void outportb_p(unsigned short port, unsigned char data)
{
  __asm__ __volatile__ ("outb %b0,%w1\noutb %%al,$0x80": :"a" (data),
                         "Nd" (port));
}

/*
 * Writes two bytes to a port
 */
void outportw(unsigned short port, unsigned short data)
{
  __asm__ __volatile__ ("outw %%ax, %%dx" : : "a" (data), "d" (port));
}

/*
 * Writes two bytes to a port and waits until it completes.
 */
void outportw_p(unsigned short port, unsigned short data)
{
  __asm__ __volatile__ ("outw %w0,%w1\noutb %%al,$0x80": :"a" (data),
                         "Nd" (port));
}

/*
 * Writes a long (4 bytes) to a port
 */
void outportl(unsigned short port, unsigned long data)
{
  __asm__ __volatile__ ("outl %%eax,%%dx" : : "a"(data), "d"(port));
}

/*
 * Writes a long (4 bytes) to a port and waits until it completes.
 */
void outportl_p(unsigned short port, unsigned long data)
{
  __asm__ __volatile__ ("outl %0,%w1\noutb %%al,$0x80": :"a" (data),
                         "Nd" (port));
}


/*
 * Writes count bytes from the address to the port.
 */
void outportsb(unsigned short port, const void * addr, unsigned long int count) {
  __asm__ __volatile__ ("cld ; rep ; outsb":"=S" (addr), "=c" (count)
                       :"d" (port), "0" (addr), "1" (count));
}

/*
 * Writes count words (x 2 bytes) from the address into the port.
 */
void outportsw(unsigned short port, const void * addr, unsigned long int count) {
  __asm__ __volatile__ ("cld ; rep ; outsw":"=S" (addr), "=c" (count)
                       :"d" (port), "0" (addr), "1" (count));
}

/*
 * Writes count longs (x 4 bytes) from the address into the port.
 */
void outportsl(unsigned short port, const void * addr, unsigned long int count) {
  __asm__ __volatile__ ("cld ; rep ; outsl":"=S" (addr), "=c" (count)
                       :"d" (port), "0" (addr), "1" (count));
}

/*
 * Sets count bytes of destination to val
 */
void *memset(void *dest, char val, unsigned int count)
{
  char *temp = (char *)dest;
  for( ; count != 0; count--) *temp++ = val;
  return dest;
}

/*
 * Copies count bytes from src to dest
 */
void * memcpy(void * dest, void * src, unsigned int count)
{
  char *temp = (char *)dest;
  char *temp2 = (char *)src;
  for( ; count != 0; count--) *temp++ = *temp2++;
  return dest;
}

/*
 * Swaps two characters in a string
 */
void swap(char * c1, char * c2)
{
  char temp;
  temp = *c1;
  *c1 = *c2;
  *c2 = temp;
}

/*
 * Reverses a null-terminated string
 */
void reverse(char * string, unsigned int length)
{
  unsigned int start = 0;
  unsigned int end = length - 1;
	
  while(start < end)
  {	
    swap(string+start,string+end);
    start++;
    end--;
  }
}

/*
 * Converts an integer into a string
 * Source http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
 * Update: only shows negative sign for base 10
 * Updated Source: http://www.geeksforgeeks.org/implement-itoa/
 */
char* itoa(int value, char *result, int base)
{
  int i = 0;
  int isNegative = 0;
	
  /* Check for valid base */
  if (base < 2 || base > 36)
  { *result = '\0'; return result; }

  if(value == 0)
  {
    result[i++] = '0';
    result[i] = '\0';
    return result;
  }
	
  if(value < 0 && base == 10)
  {
    isNegative = 1;
    value = -value;
  }
	
  do
  {
    int tmp_value = value;
    value /= base;
    result[i++] = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while ( value );
	
  if(isNegative)
    result[i++] = '-';
		
  result[i] = '\0';

  reverse(result, i);

  return result;
}

// https://github.com/bminor/newlib/blob/master/newlib/libc/stdlib/utoa.c
char * utoa (unsigned value,char *str, int base) {
    const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    int i, j;
    unsigned remainder;
    char c;

    /* Check base is supported. */
    if ((base < 2) || (base > 36))
    {
        str[0] = '\0';
        return NULL;
    }

    /* Convert to string. Digits are in reverse order.  */
    i = 0;
    do
    {
        remainder = value % base;
        str[i++] = digits[remainder];
        value = value / base;
    } while (value != 0);
    str[i] = '\0';

    /* Reverse string.  */
    for (j = 0, i--; j < i; j++, i--)
    {
        c = str[j];
        str[j] = str[i];
        str[i] = c;
    }

    return str;
}

/**
 * Given an ascii value, return the binary value in the correct base
 * @param value the ascii value
 * @param base the base
 * @return the binary value in that base
 */
char atoi(char value, int base) {
  return value - 'A'; // todo make work for bases other than 10
}

/*
 * Returns the number of bytes in the target null-terminated string
 * (not including the null character
 */
size_t strlen(const char * s)
{
  int len = 0;
  while(s[len] != '\0')
    len++;
  return len;
}

/*
 * Returns the destination string after the src is copied to it
 * src must be null-terminated and should not overlap with dest. dest
 * should also have enough space to hold src
 */
char * strcpy(char * dest, char * src)
{
  int pos = 0;
  while(*src != '\0')
  {
    dest[pos] = src[pos];
    pos++;
  }
  return dest;
}

char * strcat(char * dst, const char * src) {
    char * temp = dst;
    while (*temp != '\0') {
        temp++;
    }
    while (*src != '\0') {
        *temp = *src;
        temp++;
        src++;
    }
    return dst;
}

/*
 * Returns 0 if str1 matches exactly str2, else returns -1
 * (including size / length)
 */
int strcmp(const char *str1, const char *str2)
{
  int pos;
  int size1 = strlen(str1);
  int size2 = strlen(str2);

  if(size1 != size2)
    return -1;

  for(pos = 0; pos < size1; pos++)
  {
    if(str1[pos] != str2[pos])
      return -1;
  }

  return 0;
}

int strncmp(const char *str1, const char *str2, size_t n) {
    int pos;
    for (pos = 0; pos < n; pos++) {
        if (str1[pos] != str2[pos]) {
            return -1;
        }
    }
    return 0;
}

/*
 * Returns the pos of the first occurance of c, or -1 if end of string is
 * reached. Must be null-terminated
 */
int strpos(const char *str, const char c)
{
  int i=0;
  while(*str)
  {
    if(*str == c)
      return i;
    i++;
    str++;
  }
  return -1;
}

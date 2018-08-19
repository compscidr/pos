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
 * Reads two bytes from a port
 */
unsigned short inportw(unsigned short port)
{
  unsigned short rv;
  __asm__ __volatile__ ("inw %%dx, %%ax" : "=a" (rv) : "d" (port));
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
 * Writes a bytes to a port
 */
void outportb(unsigned short port, unsigned char data)
{
  __asm__ __volatile__ ("outb %%al,%%dx" : : "a" (data), "d" (port));
}

/*
 * Writes two bytes to a port
 */
void outportw(unsigned short port, unsigned short data)
{
  __asm__ __volatile__ ("outw %%ax, %%dx" : : "a" (data), "d" (port));
}

/*
 * Writes a long (4 bytes) to a port
 */
void outportl(unsigned short port, unsigned long data)
{
  __asm__ __volatile__ ("outl %%eax,%%dx" : : "a"(data), "d"(port));
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

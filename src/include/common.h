#ifndef COMMON_HEADER
#define COMMON_HEADER

#define NULL ((void *)0)
typedef unsigned int size_t;

unsigned char inportb(unsigned short port);
unsigned short inportw(unsigned short port);
unsigned long int inportl(unsigned short port);
void outportb(unsigned short port, unsigned char data);
void outportw(unsigned short port, unsigned short data);
void outportl(unsigned short port, unsigned long data);
void *memset(void *dest, char val, unsigned int count);
void * memcpy(void * dest, void * src, unsigned int count);
int strpos(const char *str, const char c);

void swap(char * c1, char * c2);
void reverse(char * string, unsigned int length);
char* itoa(int value, char *result, int base);
size_t strlen(const char * s);
char * strcpy(char * dest, char * src);
int strcmp(const char *str1, const char *str2);

/* Represent all of the registers */
struct regs
{
  unsigned int gs, fs, es, ds;                          /* pushed the segs last */
  unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by pusha */
  unsigned int int_no, err_code;                        /* push byte # */
  unsigned int eip, cs, eflags, useresp, ss;            /* pushed by cpu automatically */
};

#endif

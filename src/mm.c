#include "common.h"
#include "screen.h"

void * mem = (void*)0x1000000;			/* the address where we start giving out memory from */
void * lim = (void*)0x3FFFFFF;			/* the end address of the malloc space */

/*
 * allocates an entire page (4096 bytes) in kernel memory space and returns
 * a pointer to the start of the block, or NULL
 */
void * kalloc_page(void)
{
  void * loc = mem;
  mem = mem + 4096;
  if((unsigned int)mem <= (unsigned int)lim)
    return loc;
  else
    return NULL;
}

/*
 * allocates 'size' bytes of memory if there is enough available, or
 * returns NULL
 */
void * malloc(size_t size)
{
  void * loc = mem;
  mem = mem + size;
  if((unsigned int)mem <= (unsigned int)lim)
    return loc;
  else
    return NULL;
}

/*
 * allocates 'size' bytes of memory if there is enough available, or
 * returns NULL. Also the memory is zero'd out.
 */
void * calloc(size_t size)
{
  char * loc = malloc(size);	
  if(loc != NULL)
  {
    unsigned int i;
    for(i = 0; i < size; i++)
      loc[i] = 0;
  }
  return loc;
}

/*
 * Displays the amount of free memory in bytes
 */
void display_free_bytes(void)
{
  char buffer[40] = {0};
  int free = lim - mem;
  print_string("Free memory: ");
  itoa(free, buffer, 10);
  print_string(buffer);
  print_string(" bytes.\n");
}

/*
 * Returns the number of free bytes left in the system
 */
int mm_get_free(void)
{
  return lim - mem;
}

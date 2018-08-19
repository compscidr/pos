#ifndef MM_HEADER
#define MM_HEADER

void * kalloc_page(void);
void * malloc(size_t size);
void * calloc(size_t size);
void display_free_bytes(void);
int mm_get_free(void);

#endif

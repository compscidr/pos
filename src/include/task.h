#ifndef TASK_HEADER
#define TASK_HEADER

void thread_init(void);
unsigned int task_switch(unsigned int old_esp);
void create_task(void (*t)());
int fork(void);

#endif

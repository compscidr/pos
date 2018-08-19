#include "common.h"
#include "mm.h"
#include "screen.h"
#include "task.h"

struct thread {
  unsigned int id;
  unsigned int esp0;
  unsigned int esp3;
  unsigned int start_stack;
  unsigned int end_stack;
  struct thread * next_thread;
};

struct thread * thread_list;
struct thread * current_thread;

int current_id = 0;

/*
 * Initalizes threading by setting the current_thread and the thread_list to null
 */
void thread_init(void)
{
  thread_list = NULL;
  current_thread = NULL;
  create_task(NULL);			/* kludge to get multi-tasking to work - for some reason first task is always skipped!? */
}

/*
 * Creates a task given the id and the function pointer where the task should start executing
 * Source: http://hosted.cjmovie.net/TutMultitask.htm
 */
void create_task(void (*t)())
{
  struct thread * new_thread = malloc(sizeof(struct thread));
    
  if(new_thread == NULL)
    print_string("NULL THREAD from MALLOC");
    
  unsigned int *stack;
    
  /* allocate a page for the thread's stack and point to the end of it (stack grows down) */
  new_thread->id = current_id++;
  new_thread->start_stack = (unsigned int)kalloc_page();
  new_thread->end_stack = new_thread->start_stack + 4096;
  new_thread->esp0 = new_thread->end_stack;
  new_thread->next_thread = NULL;
  stack = (unsigned int*)new_thread->esp0;
  
  *--stack = 0x0202;					/* EFLAGS */
  *--stack = 0x08;					/* CS - code seg */
  *--stack = (unsigned int)t;			/* EIP */

  *--stack = 0;						/* EDI */
  *--stack = 0;						/* ESI */
  *--stack = 0;						/* EBP */
  *--stack = 0;						/* Offset */
  *--stack = 0;						/* EBX */
  *--stack = 0;						/* EDX */
  *--stack = 0;						/* ECX */
  *--stack = 0;						/* EAX */

  *--stack = 0x10;					/* DS */
  *--stack = 0x10; 					/* ES */
  *--stack = 0x10;					/* FS */
  *--stack = 0x10;					/* GS */
    
  new_thread->esp0 = (unsigned int)stack;
  struct thread * traverse = thread_list;
  
  /* no existing threads yet */
  if(traverse == NULL)
  {
    thread_list = new_thread;
    current_thread = new_thread;
  }
  else
  {
    while(traverse->next_thread != NULL)
      traverse = traverse->next_thread;
      
    traverse->next_thread = new_thread;
  }
}

/*
 * Performs the actual task switch (called by the irq0 timer handler)
 */
unsigned int task_switch(unsigned int old_esp)
{
  /*
   * if we don't have a current thread yet, just continue where we were
   */
  if(current_thread != NULL)
    current_thread->esp0 = old_esp;
  else
    return old_esp;

  /* switch to next thread in linked list, or start of list */
  current_thread = current_thread->next_thread;
  if(current_thread == NULL)
    current_thread = thread_list;

  return current_thread->esp0;
}

/*
 * Creates a new process, copies the stack from the old process
 */
int fork(void)
{
  return 0;
}

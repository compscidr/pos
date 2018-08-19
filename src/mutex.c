/*
 * This defines OS level mutexes: ie: they should be global to
 * the entire kernel. As of now (dec 3, 2013), the only code using this lock is the
 * network buffers
 * 
 * todo: actually implement more than one lock lol
 */
#define LOCKED 1
#define UNLOCKED 0

volatile int status = UNLOCKED;

void spinlock() {
  while (status == LOCKED) {}

  asm volatile("cli"); /* Disable interrupts */
  if (status == UNLOCKED)
    status = LOCKED;
  asm volatile("sti"); /* Enable interrupts */
}

void spinunlock() {

  asm volatile("cli"); /* Disable interrupts */
  status = UNLOCKED;
  asm volatile("sti"); /* Enable interrupts */
}

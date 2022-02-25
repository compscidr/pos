#include "common.h"
#include "task.h"
#include "screen.h"
#include "mm.h"

#define TIMER_MAX 1193180

unsigned int timer_hz = 18;
unsigned long int timer_ticks = 0;
unsigned long int seconds = 0;
unsigned long int days = 0;

void timer_set_tick_frequency(unsigned int hz);
void clock();

/*
 * Installs the timer
 */
void timer_init(void)
{
  timer_set_tick_frequency(1000);
  timer_ticks = 0;
  seconds = 0;
}

/*
 * Called everytime the IRQ for the timer fires
 */
unsigned int timer_handler(unsigned int old_esp)
{
  timer_ticks++;
    
  if(timer_ticks % timer_hz == 0)
  {
    seconds++;
    
    print_string_at("System Uptime: ", 0, 24);
    char time_string[35] = {0};
    char mem_string[35] = {0};
    itoa(seconds, time_string, 10);
    itoa(mm_get_free(), mem_string, 10);
    print_string_at(time_string, 15,24);
    print_string_at(" seconds.", 15 + strlen(time_string), 24);
    print_string_at(" Freemem: ", 50, 24);
    print_string_at(mem_string, 60, 24);
    print_string_at(" bytes.", 60 + strlen(mem_string), 24);
    timer_ticks = 0;
  }
  return task_switch(old_esp);
}

/*
 * Sets the resolution of our PIT timer
 */
void timer_set_tick_frequency(unsigned int hz)
{
  unsigned int div;
  unsigned char tmp;
  
  div = TIMER_MAX / hz;
  outportb(0x43, 0xb6);
  outportb(0x40, (unsigned char)div);
  outportb(0x40, (unsigned char)(div >> 8));
  
  tmp = inportb(0x61);
  if(tmp != (tmp | 3))
    outportb(0x61,tmp | 3);
  
  timer_hz = hz;
}

void sleep(int time_ms) {
  int start_ms = (seconds * 1000) + timer_ticks;
  int current_ms = start_ms;
  while (current_ms - start_ms < time_ms) {
    current_ms = (seconds * 1000) + timer_ticks;
  }
}
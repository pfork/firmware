#ifndef systimer_h
#define systimer_h

extern unsigned long long sysctr;

void sys_tick_handler(void);
void systick_init( void );
void Delay(unsigned int );
#endif
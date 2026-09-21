#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H

#include <stdint.h>

#define PIT_BASE_FREQ     1193182
#define PIT_HZ            100

#define SCHEDULER_QUANTUM 1

void pit_init(void);
uint64_t uptime_seconds(void);
void reset_quantum(void);
// void sleep()

#endif

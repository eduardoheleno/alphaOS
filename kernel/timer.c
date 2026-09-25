#include "timer.h"

#include "pic.h"
#include "misc.h"
#include "scheduler.h"

static volatile uint64_t ticks = 0;
static uint8_t quantum_tick = 0;

void pit_init(void)
{
    uint16_t divisor = PIT_BASE_FREQ / PIT_HZ;

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
}

void reset_quantum(void)
{
    quantum_tick = 0;
}

void timer_interrupt_handler(cpu_task_state_t* state)
{
    ticks++;
    quantum_tick++;

    if (quantum_tick >= SCHEDULER_QUANTUM)
    {
        reset_quantum();
        scheduler_tick(state);
        return;
    }

    pic_send_eoi(0);
}

uint64_t uptime_seconds(void)
{
    return ticks / PIT_HZ;
}

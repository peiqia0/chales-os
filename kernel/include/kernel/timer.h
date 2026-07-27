#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H

#include <stdint.h>

// PIT (Programmable Interval Timer) ports
#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND_REG 0x43

// Timer frequency (1000 Hz = 1ms per tick)
#define TIMER_FREQUENCY 1000

// Initialize PIT timer
void timer_initialize(void);

// Called on each timer interrupt
void timer_tick(void);

// Get system ticks since boot
uint32_t timer_get_ticks(void);

// Get elapsed time in milliseconds
uint32_t timer_get_ms(void);

#endif

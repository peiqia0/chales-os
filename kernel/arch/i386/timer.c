#include <stdint.h>

#include <kernel/portio.h>

// PIT (Programmable Interval Timer) ports
#define PIT_CHANNEL_0 0x40
#define PIT_COMMAND_REG 0x43

// PIT command byte
// Bits 7-6: channel (00 = channel 0)
// Bits 5-4: access mode (11 = lobyte/hibyte)
// Bits 3-1: operating mode (010 = rate generator/timer)
// Bit 0: bcd mode (0 = 16-bit binary)
#define PIT_CMD_CHANNEL_0 0x00
#define PIT_CMD_ACCESS_LOHI 0x30
#define PIT_CMD_MODE_2 0x04
#define PIT_CMD_BINARY 0x00
#define PIT_CMD_INIT (PIT_CMD_CHANNEL_0 | PIT_CMD_ACCESS_LOHI | PIT_CMD_MODE_2 | PIT_CMD_BINARY)

// Base PIT frequency (1193182 Hz)
#define PIT_BASE_FREQUENCY 1193182

// Timer frequency (1000 Hz = 1ms per tick)
#define TIMER_FREQUENCY 1000

// System tick counter
static uint32_t system_ticks = 0;

void timer_initialize(void)
{
    // Calculate divisor for desired frequency
    uint16_t divisor = PIT_BASE_FREQUENCY / TIMER_FREQUENCY;

    // Send command byte to PIT
    outport8(PIT_COMMAND_REG, PIT_CMD_INIT);

    // Send divisor (low byte first, then high byte)
    outport8(PIT_CHANNEL_0, divisor & 0xFF);
    outport8(PIT_CHANNEL_0, (divisor >> 8) & 0xFF);

    system_ticks = 0;
}

void timer_tick(void)
{
    system_ticks++;
}

uint32_t timer_get_ticks(void)
{
    return system_ticks;
}

uint32_t timer_get_ms(void)
{
    // Each tick is 1ms with TIMER_FREQUENCY = 1000
    return system_ticks;
}

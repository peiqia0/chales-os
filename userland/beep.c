#include <syscalls.h>
#include "ulib.h"

/* Drives the PC speaker directly through the raw port-I/O syscalls
 * (_syscall_outport8/_syscall_inport8) — the classic PIT channel 2 +
 * speaker gate trick. Demonstrates that a loaded ELF can do real
 * hardware I/O, not just talk to ramfs/tty through higher-level
 * syscalls. This only makes sound under QEMU with audio enabled (or on
 * real hardware) — headless/CI runs will just see the print output. */

#define PIT_CHANNEL2 0x42
#define PIT_COMMAND  0x43
#define PIT_FREQ_HZ  1193182u
#define SPEAKER_PORT 0x61

static void speaker_on(unsigned int frequency)
{
    unsigned int divisor = PIT_FREQ_HZ / frequency;

    _syscall_outport8(PIT_COMMAND, 0xB6); // channel 2, mode 3 (square wave)
    _syscall_outport8(PIT_CHANNEL2, (unsigned char)(divisor & 0xFF));
    _syscall_outport8(PIT_CHANNEL2, (unsigned char)((divisor >> 8) & 0xFF));

    unsigned char tmp = _syscall_inport8(SPEAKER_PORT);
    if ((tmp & 0x3) != 0x3) {
        _syscall_outport8(SPEAKER_PORT, tmp | 0x3); // gate the speaker on
    }
}

static void speaker_off(void)
{
    unsigned char tmp = _syscall_inport8(SPEAKER_PORT);
    _syscall_outport8(SPEAKER_PORT, tmp & 0xFC);
}

void _start(void)
{
    uprint("beep: 880Hz for ~30 ticks...\n");

    speaker_on(880);
    usleep_ticks(30);
    speaker_off();

    uprint("beep: done.\n");
    _syscall_exit(0);
}

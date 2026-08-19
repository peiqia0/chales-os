#include <syscalls.h>
#include "ulib.h"

// Demonstrates _syscall_get_ticks() an ELF.

void _start(void)
{
    unsigned long ticks = _syscall_get_ticks();

    uprint("uptime: ");
    uprint_uint(ticks);
    uprint(" timer ticks since boot\n");

    _syscall_exit(0);
}

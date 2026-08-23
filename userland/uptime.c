#include <syscalls.h>
#include <stdio.h>

// Demonstrates _syscall_get_ticks() an ELF.

void _start(void)
{
    unsigned long ticks = _syscall_get_ticks();

    printf("uptime: %lu timer ticks since boot\n", ticks);

    _syscall_exit(0);
}

#include <syscalls.h>
#include <stdio.h>

// Demonstrates _syscall_get_ticks() an ELF.

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    unsigned long ticks = _syscall_get_ticks();

    printf("uptime: %lu timer ticks since boot\n", ticks);

    return 0;
}

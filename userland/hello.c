#include <syscalls.h>
#include <stdio.h>


void _start(void)
{
    printf("Hello from an ELF-loaded user program!\n");
    _syscall_exit(0);

    for (;;) {
    }
}

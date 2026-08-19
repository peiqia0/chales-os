#include <syscalls.h>

static void print(const char *s)
{
    while (*s) {
        _syscall_putchar((int)*s++);
    }
}


void _start(void)
{
    print("Hello from an ELF-loaded user program!\n");

    _syscall_exit(0);

    for (;;) {
    }
}

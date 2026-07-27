#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#include <stdint.h>

static inline int _syscall_putchar(int c)
{
    asm volatile (
        "int $0x80\n\t"
        :
        : "a"(1), "b"(c)
        : "memory"
    );

    return c;
}

static inline int _syscall_getchar(void)
{
    int ch;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(ch)
        : "a"(2)
        : "memory"
    );
    return ch;
}

static inline void _syscall_exit(int code)
{
    asm volatile (
        "int $0x80\n\t"
        :
        : "a"(0), "b"(code)
        : "memory"
    );
}

static inline unsigned long _syscall_get_ticks(void)
{
    unsigned long ticks;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(ticks)
        : "a"(3)
        : "memory"
    );
    return ticks;
}

/* Grows (or shrinks, if increment is negative — not currently supported by
 * the kernel side) the calling process's heap by `increment` bytes.
 * Returns the *previous* break (i.e. the start of the newly-allocated
 * region) on success, or (void*)-1 if the kernel refused (heap exhausted).
 * Standard sbrk() semantics, minus negative increments for now.
 */
static inline void* _syscall_sbrk(int increment)
{
    void* prev_brk;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(prev_brk)
        : "a"(4), "b"(increment)
        : "memory"
    );
    return prev_brk;
}

#endif
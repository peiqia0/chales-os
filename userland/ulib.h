#ifndef _ULIB_H
#define _ULIB_H

#include <syscalls.h>

static inline void uprint(const char *s)
{
    while (*s) {
        _syscall_putchar((int)(unsigned char)*s);
        s++;
    }
}

static inline void uprint_uint(unsigned long v)
{
    char buf[11]; // up to 10 digits
    int i = 10;

    buf[i] = '\0';

    if (v == 0) {
        uprint("0");
        return;
    }

    while (v > 0 && i > 0) {
        buf[--i] = (char)('0' + (v % 10));
        v /= 10;
    }

    uprint(&buf[i]);
}

static inline void uprint_hex(unsigned long v)
{
    static const char digits[] = "0123456789abcdef";
    char buf[9];
    int i;

    buf[8] = '\0';
    for (i = 7; i >= 0; i--) {
        buf[i] = digits[v & 0xF];
        v >>= 4;
    }

    uprint("0x");
    uprint(buf);
}

static inline void usleep_ticks(unsigned long n)
{
    unsigned long start = _syscall_get_ticks();
    while (_syscall_get_ticks() - start < n) {
    }
}

#endif

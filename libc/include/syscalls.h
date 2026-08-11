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

static inline int _syscall_outport8(uint16_t port, uint8_t value)
{
    int result;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(result)
        : "a"(14), "b"(port), "c"(value)
        : "memory"
    );
    return result;
}

static inline int _syscall_outport16(uint16_t port, uint16_t value)
{
    int result;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(result)
        : "a"(15), "b"(port), "c"(value)
        : "memory"
    );
    return result;
}

static inline int _syscall_outport32(uint16_t port, uint32_t value)
{
    int result;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(result)
        : "a"(16), "b"(port), "c"(value)
        : "memory"
    );
    return result;
}

static inline uint8_t _syscall_inport8(uint16_t port)
{
    int result;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(result)
        : "a"(17), "b"(port)
        : "memory"
    );
    return result;
}

static inline uint16_t _syscall_inport16(uint16_t port)
{
    int result;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(result)
        : "a"(18), "b"(port)
        : "memory"
    );
    return result;
}

static inline uint32_t _syscall_inport32(uint16_t port)
{
    int result;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(result)
        : "a"(19), "b"(port)
        : "memory"
    );
    return result;
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


static inline void _syscall_ls(const char* path)
{
    asm volatile (
        "int $0x80\n\t"
        :
        : "a"(5), "b"(path)
        : "memory"
    );
}

static inline int _syscall_open(const char* path, int flags)
{
    int fd;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(fd)
        : "a"(6), "b"(path), "c"(flags)
        : "memory"
    );
    return fd;
}

static inline int _syscall_close(int fd)
{
    int ret;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(ret)
        : "a"(7), "b"(fd)
        : "memory"
    );
    return ret;
}

static inline int _syscall_read(int fd, void* buf, unsigned int count)
{
    int ret;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(ret)
        : "a"(8), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

static inline int _syscall_write(int fd, const void* buf, unsigned int count)
{
    int ret;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(ret)
        : "a"(9), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}

static inline int _syscall_mkdir(const char* path)
{
    int ret;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(ret)
        : "a"(10), "b"(path)
        : "memory"
    );
    return ret;
}

static inline int _syscall_rmdir(const char* path)
{
    int ret;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(ret)
        : "a"(12), "b"(path)
        : "memory"
    );
    return ret;
}

static inline int _syscall_unlink(const char* path)
{
    int ret;
    asm volatile (
        "int $0x80\n\t"
        : "=a"(ret)
        : "a"(11), "b"(path)
        : "memory"
    );
    return ret;
}

static inline void _syscall_clear(void)
{
    asm volatile (
        "int $0x80\n\t"
        :
        : "a"(13)
        : "memory"
    );
}

/* Does not return. */
static inline void _syscall_reboot(void)
{
    asm volatile (
        "int $0x80\n\t"
        :
        : "a"(20)
        : "memory"
    );
}

#endif

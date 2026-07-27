#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <syscalls.h>

static void _syscall_print_string(const char *str)
{
    while (*str) {
        _syscall_putchar((unsigned char)*str++);
    }
}

static void _syscall_print_decimal(unsigned long value)
{
    char buffer[16];
    int index = 0;

    if (value == 0) {
        _syscall_putchar('0');
        return;
    }

    while (value > 0 && index < (int)sizeof(buffer) - 1) {
        buffer[index++] = '0' + (value % 10);
        value /= 10;
    }

    while (index > 0) {
        _syscall_putchar(buffer[--index]);
    }
}

static void _syscall_print_hex(unsigned long value, bool uppercase)
{
    char buffer[16];
    int index = 0;

    if (value == 0) {
        _syscall_putchar('0');
        return;
    }

    while (value > 0 && index < (int)sizeof(buffer) - 1) {
        unsigned int digit = value & 0xF;
        buffer[index++] = digit < 10 ? '0' + digit
                                     : (uppercase ? 'A' : 'a') + (digit - 10);
        value >>= 4;
    }

    while (index > 0) {
        _syscall_putchar(buffer[--index]);
    }
}

int printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    for (const char *p = format; *p; ++p) {
        if (*p != '%') {
            _syscall_putchar((unsigned char)*p);
            continue;
        }

        ++p;
        if (*p == '\0')
            break;

        switch (*p) {
            case 's':
            {
                const char *str = va_arg(args, const char *);
                _syscall_print_string(str ? str : "(null)");
                break;
            }
            case 'c':
            {
                int ch = va_arg(args, int);
                _syscall_putchar(ch);
                break;
            }
            case 'u':
            {
                unsigned int value = va_arg(args, unsigned int);
                _syscall_print_decimal(value);
                break;
            }
            case 'x':
            {
                unsigned int value = va_arg(args, unsigned int);
                _syscall_print_hex(value, false);
                break;
            }
            case 'X':
            {
                unsigned int value = va_arg(args, unsigned int);
                _syscall_print_hex(value, true);
                break;
            }
            case 'd': case 'i':
            {
                int value = va_arg(args, int);
                if (value < 0) {
                    _syscall_putchar('-');
                    _syscall_print_decimal((unsigned int)(-value));
                } else {
                    _syscall_print_decimal((unsigned int)value);
                }
                break;
            }
            case 'l':
            {
                ++p;
                if (*p == 'u') {
                    unsigned long value = va_arg(args, unsigned long);
                    _syscall_print_decimal(value);
                } else if (*p == 'd') {
                    long value = va_arg(args, long);
                    if (value < 0) {
                        _syscall_putchar('-');
                        _syscall_print_decimal((unsigned long)(-value));
                    } else {
                        _syscall_print_decimal((unsigned long)value);
                    }
                } else if (*p == 'x') {
                    unsigned long value = va_arg(args, unsigned long);
                    _syscall_print_hex(value, false);
                } else if (*p == 'X') {
                    unsigned long value = va_arg(args, unsigned long);
                    _syscall_print_hex(value, true);
                } else {
                    _syscall_putchar('l');
                    if (*p) _syscall_putchar((unsigned char)*p);
                }
                break;
            }
            case '%':
                _syscall_putchar('%');
                break;
            default:
                _syscall_putchar('%');
                _syscall_putchar((unsigned char)*p);
                break;
        }
    }

    va_end(args);
    return 0;
}

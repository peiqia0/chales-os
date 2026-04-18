#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

static bool print(const char* data, size_t length) {
    for (size_t i = 0; i < length; i++)
        if (putchar((unsigned char)data[i]) == EOF)
            return false;
    return true;
}

// unsigned int to string
static size_t utoa_custom(unsigned int value, char* buffer, unsigned int base, const char* digits) {
    char temp[32];
    size_t i = 0;

    if (value == 0) {
        buffer[0] = '0';
        return 1;
    }

    while (value > 0) {
        temp[i++] = digits[value % base];
        value /= base;
    }

    // reverse
    for (size_t j = 0; j < i; j++)
        buffer[j] = temp[i - j - 1];

    return i;
}

// unsigned int with padding
static size_t utoa_padded_custom(unsigned int value, char* buffer, unsigned int base, const char* digits, int width, char pad_char) {
    size_t len = utoa_custom(value, buffer, base, digits);

    if (width > 0 && (size_t)width > len) {
        size_t pad = width - len;
        // shift digits
        for (size_t i = len; i > 0; i--)
            buffer[i + pad - 1] = buffer[i - 1];
        for (size_t i = 0; i < pad; i++)
            buffer[i] = pad_char;
        len = width;
    }

    return len;
}

// unsigned long to string
static size_t ultoa_custom(unsigned long value, char* buffer, unsigned int base, const char* digits) {
    char temp[32];
    size_t i = 0;

    if (value == 0) {
        buffer[0] = '0';
        return 1;
    }

    while (value > 0) {
        temp[i++] = digits[value % base];
        value /= base;
    }

    for (size_t j = 0; j < i; j++)
        buffer[j] = temp[i - j - 1];

    return i;
}

// unsigned long with padding
static size_t ultoa_padded_custom(unsigned long value, char* buffer, unsigned int base, const char* digits, int width, char pad_char) {
    size_t len = ultoa_custom(value, buffer, base, digits);

    if (width > 0 && (size_t)width > len) {
        size_t pad = width - len;
        for (size_t i = len; i > 0; i--)
            buffer[i + pad - 1] = buffer[i - 1];
        for (size_t i = 0; i < pad; i++)
            buffer[i] = pad_char;
        len = width;
    }

    return len;
}

// signed int
static size_t itoa(int value, char* buffer, int width, char pad_char) {
    size_t i = 0;
    unsigned int v;

    if (value < 0) {
        buffer[i++] = '-';
        v = (unsigned int)(-value);
    } else {
        v = (unsigned int)value;
    }

    size_t len = utoa_padded_custom(v, buffer + i, 10, "0123456789", width - (int)i, pad_char);
    return i + len;
}

int printf(const char* restrict format, ...) {
    va_list parameters;
    va_start(parameters, format);

    int written = 0;

    while (*format) {
        size_t maxrem = INT_MAX - written;

        if (format[0] != '%' || format[1] == '%') {
            if (format[0] == '%') format++;
            size_t amount = 1;
            while (format[amount] && format[amount] != '%') amount++;
            if (maxrem < amount) return -1;
            if (!print(format, amount)) return -1;
            format += amount;
            written += amount;
            continue;
        }

        const char* format_begun_at = format++;
        char buf[64];

        // parse padding/width
        char pad_char = ' ';
        int width = 0;
        if (*format == '0') {
            pad_char = '0';
            format++;
        }
        while (*format >= '0' && *format <= '9') {
            width = width * 10 + (*format - '0');
            format++;
        }

        switch (*format) {
            case 'c': {
                format++;
                char c = (char)va_arg(parameters, int);
                if (!maxrem || !print(&c, 1)) return -1;
                written++;
                break;
            }
            case 's': {
                format++;
                const char* str = va_arg(parameters, const char*);
                size_t len = strlen(str);
                if (maxrem < len || !print(str, len)) return -1;
                written += len;
                break;
            }
            case 'd': case 'i': {
                format++;
                int v = va_arg(parameters, int);
                size_t len = itoa(v, buf, width, pad_char);
                if (maxrem < len || !print(buf, len)) return -1;
                written += len;
                break;
            }
            case 'u': {
                format++;
                unsigned int v = va_arg(parameters, unsigned int);
                size_t len = utoa_padded_custom(v, buf, 10, "0123456789", width, pad_char);
                if (maxrem < len || !print(buf, len)) return -1;
                written += len;
                break;
            }
            case 'x': case 'X': {
                int uppercase = (*format == 'X');
                format++;
                unsigned int v = va_arg(parameters, unsigned int);
                const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
                size_t len = utoa_padded_custom(v, buf, 16, digits, width, pad_char);
                if (maxrem < len || !print(buf, len)) return -1;
                written += len;
                break;
            }
            case 'l': {
                format++;
                if (*format == 'u') {
                    format++;
                    unsigned long v = va_arg(parameters, unsigned long);
                    size_t len = ultoa_padded_custom(v, buf, 10, "0123456789", width, pad_char);
                    if (maxrem < len || !print(buf, len)) return -1;
                    written += len;
                } else if (*format == 'x' || *format == 'X') {
                    int uppercase = (*format == 'X');
                    format++;
                    unsigned long v = va_arg(parameters, unsigned long);
                    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
                    size_t len = ultoa_padded_custom(v, buf, 16, digits, width, pad_char);
                    if (maxrem < len || !print(buf, len)) return -1;
                    written += len;
                }
                break;
            }
            case 'p': {
                format++;
                uintptr_t v = (uintptr_t)va_arg(parameters, void*);
                const char prefix[] = "0x";
                size_t len1 = 2;
                size_t len2 = ultoa_custom((unsigned long)v, buf, 16, "0123456789abcdef");
                if (maxrem < len1 + len2) return -1;
                if (!print(prefix, 2) || !print(buf, len2)) return -1;
                written += len1 + len2;
                break;
            }
            default: {
                format = format_begun_at;
                size_t len = strlen(format);
                if (maxrem < len || !print(format, len)) return -1;
                written += len;
                format += len;
                break;
            }
        }
    }

    va_end(parameters);
    return written;
}

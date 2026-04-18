#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct {
	char* buffer;
	size_t position;
	size_t size;
} snprintf_state_t;

static bool snprintf_putchar(char c, snprintf_state_t* state)
{
	if (state->position >= state->size - 1)
		return false;
	state->buffer[state->position++] = c;
	return true;
}

static bool snprintf_print(const char* data, size_t length, snprintf_state_t* state)
{
	for (size_t i = 0; i < length; i++) {
		if (!snprintf_putchar(data[i], state))
			return false;
	}
	return true;
}

static size_t utoa(unsigned int value, char* buffer, unsigned int base)
{
	static const char digits[] = "0123456789abcdef";
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

static size_t utoa_padded(unsigned int value, char* buffer, unsigned int base, int width, char pad_char)
{
	size_t len = utoa(value, buffer, base);
	
	if (width > 0 && (size_t)width > len) {
		// Shift existing digits to the right
		for (int i = len - 1; i >= 0; i--) {
			buffer[i + width - len] = buffer[i];
		}
		// Fill padding
		for (int i = 0; i < width - (int)len; i++) {
			buffer[i] = pad_char;
		}
		return width;
	}
	
	return len;
}

static size_t itoa(int value, char* buffer)
{
	size_t i = 0;
	unsigned int v;

	if (value < 0) {
		buffer[i++] = '-';
		v = (unsigned int)(-value);
	} else {
		v = (unsigned int)value;
	}

	return i + utoa(v, buffer + i, 10);
}

int snprintf(char* restrict buffer, size_t size, const char* restrict format, ...)
{
	if (size == 0)
		return -1;

	va_list parameters;
	va_start(parameters, format);

	snprintf_state_t state = {
		.buffer = buffer,
		.position = 0,
		.size = size
	};

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		// Normal characters and %% 
		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;

			if (maxrem < amount)
				return -1;

			if (!snprintf_print(format, amount, &state))
				goto overflow;

			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;
		char buf[64];

		// Parse width and padding
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

		// Format parsing

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int);
			if (!maxrem || !snprintf_putchar(c, &state))
				goto overflow;
			written++;

		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len || !snprintf_print(str, len, &state))
				goto overflow;
			written += len;

		} else if (*format == 'd' || *format == 'i') {
			format++;
			int v = va_arg(parameters, int);
			size_t len = itoa(v, buf);
			if (maxrem < len || !snprintf_print(buf, len, &state))
				goto overflow;
			written += len;

		} else if (*format == 'u' || *format == 'l') {
			// Handle %u and %lu (both treated as unsigned int for now)
			if (*format == 'l') {
				format++;
				if (*format != 'u') {
					format = format_begun_at;
					size_t len = strlen(format);
					if (maxrem < len || !snprintf_print(format, len, &state))
						goto overflow;
					written += len;
					format += len;
					continue;
				}
			}
			format++;
			unsigned int v = va_arg(parameters, unsigned int);
			size_t len = utoa_padded(v, buf, 10, width, pad_char);
			if (maxrem < len || !snprintf_print(buf, len, &state))
				goto overflow;
			written += len;

		} else if (*format == 'x') {
			format++;
			unsigned int v = va_arg(parameters, unsigned int);
			size_t len = utoa(v, buf, 16);
			if (maxrem < len || !snprintf_print(buf, len, &state))
				goto overflow;
			written += len;

		} else if (*format == 'p') {
			format++;
			uintptr_t v = (uintptr_t)va_arg(parameters, void*);
			const char prefix[] = "0x";
			size_t len1 = 2;
			size_t len2 = utoa((unsigned int)v, buf, 16);

			if (maxrem < (len1 + len2))
				goto overflow;
			if (!snprintf_print(prefix, 2, &state) || !snprintf_print(buf, len2, &state))
				goto overflow;
			written += len1 + len2;

		} else {
			/* Unknown format → print literally */
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len || !snprintf_print(format, len, &state))
				goto overflow;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	state.buffer[state.position] = '\0';
	return written;

overflow:
	va_end(parameters);
	state.buffer[state.position < state.size - 1 ? state.position : state.size - 1] = '\0';
	return -1;
}

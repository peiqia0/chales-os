#ifndef _STDIO_H
#define _STDIO_H 1

#include <sys/cdefs.h>
#include <stddef.h>

#define EOF (-1)

int printk(const char* __restrict, ...);
int printf(const char* __restrict, ...);
int snprintk(char* __restrict, size_t, const char* __restrict, ...);
int putchar(int);
int puts(const char*);

#endif

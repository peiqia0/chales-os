#include <stdio.h>

int puts(const char* string) {
    int written = printk("%s\n", string);
    return written;
}

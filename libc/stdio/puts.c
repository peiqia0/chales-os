#include <stdio.h>

int puts(const char* string) {
    int written = printf("%s\n", string);
    return written;
}

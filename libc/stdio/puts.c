#include <stdio.h>

int puts(const char* string) {
	return printk("%s\n", string);
}

#include <stdio.h>
#include <stdlib.h>

__attribute__((__noreturn__))
void abort(void) {
#if defined(__is_libk)
	printk("kernel: panic: abort()\n");
#else
	printk("abort()\n");
#endif
	while (1) { }
	__builtin_unreachable();
}

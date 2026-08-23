#include <stdio.h>
#include <stdlib.h>

#if !defined(__is_libk)
#include <syscalls.h>
#endif

__attribute__((__noreturn__))
void abort(void) {
#if defined(__is_libk)
	printk("kernel: panic: abort()\n");
	while (1) { }
#else
	puts("abort()");
	_syscall_exit(1);
	while (1) { }
#endif
	__builtin_unreachable();
}

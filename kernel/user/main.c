#include <stddef.h>
#include <syscalls.h>
#include <stdint.h>

void user_main(void)
{
    _syscall_execv("/bin/shell", NULL, NULL);
    _syscall_exit(0);
}

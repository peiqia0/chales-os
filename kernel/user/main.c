#include <stddef.h>
#include <syscalls.h>
#include <stdint.h>

void user_main(void)
{
    _syscall_exec("/bin/shell");
    _syscall_exit(0);
}

#include <stddef.h>
#include <syscalls.h>
#include <kernel/shell.h>
#include <stdint.h>

void user_main(void)
{
    shell_run();
    _syscall_exit(0);
}

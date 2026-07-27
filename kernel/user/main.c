#include <stddef.h>
#include <stdio.h>
#include <syscalls.h>
#include <kernel/shell.h>

void user_main(void)
{
    shell_run();
    _syscall_exit(0);
}

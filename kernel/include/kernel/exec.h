#ifndef KERNEL_EXEC_H
#define KERNEL_EXEC_H

#include <stdint.h>
#include <kernel/sched.h>

// Where an ELF program's user stack lives (just below the 0x300000 image).
#define EXEC_STACK_TOP  0x2FF000u
#define EXEC_STACK_SIZE 0x4000u

int exec_run(const char *path, char **argv, char **envp);

// Called when `child` exits: put back what its exec_run() saved.
void exec_restore(task_t *child);

#endif

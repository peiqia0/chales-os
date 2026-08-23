#include <stdint.h>

#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

enum states {
    TASK_RUNNING,
    TASK_READY,
    TASK_DEAD,
};

typedef struct task {
    uint32_t        esp;
    uint32_t       *stack;
    uint32_t        stack_size;
    enum states     state;
    struct task    *next;
} task_t;

extern task_t *current_task;

void sched_init(task_t *task0);
void schedule(void);
void sched_task_create(uint32_t entry, uint32_t user_stack_top);
void sched_task_kill(task_t *task);

void sched_switch(task_t *prev, task_t *next);
#endif

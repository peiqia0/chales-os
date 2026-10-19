#include <stdint.h>

#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

enum states {
    TASK_RUNNING,
    TASK_READY,
    TASK_DEAD,
    TASK_BLOCKED,   /* waiting for a child started by execv to finish */
};

typedef struct task {
    uint32_t        esp;
    uint32_t       *stack;
    uint32_t        stack_size;
    enum states     state;
    uint32_t        pid;
    struct task    *next;
    int             heap_alloc;   /* task_t and stack came from kmalloc (reapable) */

    /* Process tree / exec-and-wait */
    struct task    *parent;       /* task blocked in execv waiting for us */
    int             zombie_valid; /* an exited child waits to be reaped by wait() */
    uint32_t        zombie_pid;
    int             zombie_status;

    /* Memory image of an ELF task: [elf_lo, elf_hi). 0/0 for non-ELF tasks. */
    uint32_t        elf_lo;
    uint32_t        elf_hi;

    /* What this task's execv saved from its parent, restored when we exit. */
    int             has_frame;
    uint32_t        frame_off;    /* offset into the save pool */
    uint32_t        frame_stack_len;
    uint32_t        frame_img_base;
    uint32_t        frame_img_len;
    uint8_t        *frame_brk;    /* user heap break at exec time */
} task_t;

extern task_t *current_task;

void sched_init(task_t *task0);
void schedule(void);

/* Create a READY task that starts in user mode at entry/user_stack_top and link
 * it into the ring, without switching to it. NULL if out of memory. */
task_t *sched_task_new(uint32_t entry, uint32_t user_stack_top);
/* sched_task_new() followed by schedule() (parent keeps running state READY). */
void sched_task_create(uint32_t entry, uint32_t user_stack_top);


void sched_task_exit(task_t *task, int status);
void sched_task_kill(task_t *task);
task_t *sched_find_task(uint32_t pid);

void sched_switch(task_t *prev, task_t *next);
#endif

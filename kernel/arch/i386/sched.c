#include <kernel/sched.h>
#include <kernel/exec.h>
#include <kernel/usermode.h>
#include <kernel/kmalloc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern void tss_set_kernel_stack(uint32_t esp0);
task_t *current_task = NULL;
static uint32_t next_pid = 1;

void sched_init(task_t *task0) {
    task0->state = TASK_RUNNING;
    task0->pid = next_pid++;
    task0->next = task0;
    current_task = task0;
}

// scheduler
void schedule() {
    if (current_task == NULL) return;
    task_t *next_task = current_task->next;
    task_t *prev = current_task;
    while (next_task != current_task) {

        if (next_task->state == TASK_READY) {
            if (prev->state == TASK_RUNNING) {
                prev->state = TASK_READY;
            }
            current_task = next_task;
            next_task->state = TASK_RUNNING;

            tss_set_kernel_stack((uint32_t)next_task->stack + next_task->stack_size);
            sched_switch(prev, next_task);

            return;
        }else {
            next_task = next_task->next;
        }
    }
}

task_t *sched_find_task(uint32_t pid) {
    if (current_task == NULL) return NULL;
    task_t *t = current_task;
    do {
        if (t->pid == pid && t->state != TASK_DEAD) return t;
        t = t->next;
    } while (t != current_task);
    return NULL;
}


static void sched_reap(void) {
    task_t *prev = current_task;
    task_t *t = current_task->next;
    while (t != current_task) {
        task_t *next = t->next;
        if (t->state == TASK_DEAD && t->heap_alloc) {
            prev->next = next;
            kfree(t->stack);
            kfree(t);
        } else {
            prev = t;
        }
        t = next;
    }
}

/* Everything in sched_task_exit() except the final switch-away. */
static void task_exit_noswitch(task_t *task, int status) {
    if (task->state == TASK_DEAD) return;

    /* Descendants first, so their saved frames are popped in LIFO order. */
    for (task_t *t = task->next; t != task; t = t->next) {
        if (t->parent == task && t->state != TASK_DEAD) {
            task_exit_noswitch(t, 9 /* killed by SIGKILL */);
        }
    }

    task_t *parent = task->parent;
    if (parent) {
        exec_restore(task);
        parent->zombie_valid = 1;
        parent->zombie_pid = task->pid;
        parent->zombie_status = status;
        if (parent->state == TASK_BLOCKED) {
            parent->state = TASK_READY;
        }
    }
    task->state = TASK_DEAD;
}

void sched_task_exit(task_t *task, int status) {
    task_exit_noswitch(task, status);
    if (current_task->state == TASK_DEAD) {
        schedule();
        /* schedule() only comes back if nothing else is runnable. */
        printk("[sched] no runnable tasks left, halting\n");
        asm volatile("cli");
        for (;;) {
            asm volatile("hlt");
        }
    }
}

void sched_task_kill(task_t *task) {
    sched_task_exit(task, 9);
}

task_t *sched_task_new(uint32_t entry, uint32_t user_stack_top) {
    sched_reap();

    task_t *task = kmalloc(sizeof(task_t));
    if (!task) return NULL;
    uint32_t *stack = kmalloc(4096);
    if (!stack) {
        kfree(task);
        return NULL;
    }
    memset(task, 0, sizeof(*task));
    task->stack = stack;
    task->stack_size = 4096;
    task->heap_alloc = 1;
    uint32_t *sp = stack + (4096 / sizeof(uint32_t));
    *--sp = user_stack_top;       // arg2
    *--sp = entry;                // arg1
    *--sp = 0;                    // dummy
    *--sp = (uint32_t)enter_user_mode_at;  // return address
    *--sp = 0;                    // ebp
    *--sp = 0;                    // edi
    *--sp = 0;                    // esi
    *--sp = 0;                    // ebx

    task->esp = (uint32_t)sp;
    task->state = TASK_READY;
    task->pid = next_pid++;
    task->next = current_task->next;
    current_task->next = task;
    return task;
}

void sched_task_create(uint32_t entry, uint32_t user_stack_top) {
    if (sched_task_new(entry, user_stack_top)) {
        schedule();
    }
}

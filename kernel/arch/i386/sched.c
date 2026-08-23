#include <kernel/sched.h>
#include <kernel/usermode.h>
#include <kernel/kmalloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
extern void tss_set_kernel_stack(uint32_t esp0);
task_t *current_task = NULL;

void sched_init(task_t *task0) {
    task0->state = TASK_RUNNING;
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
            if (prev->state != TASK_DEAD){
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

void sched_task_kill(task_t *task) {

    if (task->state != TASK_DEAD) {
        task->state = TASK_DEAD;
        if (task == current_task) {
            schedule();

        }
    }

}
void sched_task_create(uint32_t entry, uint32_t user_stack_top) {
    task_t *task = kmalloc(sizeof(task_t));
    uint32_t *stack = kmalloc(4096);
    task->stack = stack;
    task->stack_size = 4096;
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
    task->next = current_task->next;
    current_task->next = task;

    schedule();
}

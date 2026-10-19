#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/tty.h>
#include <kernel/keyboard.h>
#include <kernel/timer.h>
#include <kernel/ramfs.h>
#include <kernel/usermode.h>
#include <kernel/sched.h>

#include "../arch/i386/idt.h"
#include "../arch/i386/pic.h"
#include "../arch/i386/paging.h"

extern void gdt_set_tss_base(void);
extern uint8_t stack_bottom[];
extern uint8_t stack_top[];

void kernel_early(void)
{
	terminal_initialize();
}

void kernel_main(void)
{
    printk("[IDT] initializing\n");
    idt_initialize();

    printk("[PIC] initializing\n");
    pic_initialize();

    printk("[timer] initializing\n");
    timer_initialize();

    printk("[irq] enabling timer and keyboard\n");
    pic_clear_mask(0); // IRQ0: PIT
    pic_clear_mask(1); // IRQ1: keyboard

    printk("[paging] preparing page tables\n");
    paging_initialize();
    printk("[paging] enabling paging\n");
    paging_enable();

    printk("[cpu] enabling interrupts\n");
    asm volatile("sti");

    printk("[KERNEL] initialization complete\n");

    ramfs_init();

    gdt_set_tss_base();
    static task_t task0;

    task0.state = TASK_RUNNING;
    task0.next = &task0;
    task0.stack = (uint32_t*)stack_bottom;
    task0.stack_size = (uint32_t)(stack_top - stack_bottom);
    sched_init(&task0);

    enter_user_mode();
}

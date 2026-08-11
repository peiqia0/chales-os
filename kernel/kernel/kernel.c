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

#include "../arch/i386/idt.h"
#include "../arch/i386/pic.h"
#include "../arch/i386/paging.h"


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
    // Test Ring 3 user mode
    enter_user_mode();
}

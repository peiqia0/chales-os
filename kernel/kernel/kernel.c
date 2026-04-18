#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/tty.h>
#include <kernel/keyboard.h>
#include <kernel/timer.h>
#include <kernel/ramfs.h>
#include <kernel/shell.h>

#include "../arch/i386/idt.h"
#include "../arch/i386/pic.h"


void kernel_early(void)
{
	terminal_initialize();
}

void kernel_main(void)
{
    printf("IDT: initializing\n");
    idt_initialize();

    printf("PIC: initializing\n");
    pic_initialize();

    printf("kbd: initializing\n");
    keyboard_initialize();

    printf("timer: initializing\n");
    timer_initialize();

    printf("irq: enabling timer and keyboard\n");
    pic_clear_mask(0);  // IRQ0: PIT
    pic_clear_mask(1);  // IRQ1: keyboard

    printf("cpu: enabling interrupts\n");
    asm volatile("sti");

    printf("kernel: initialization complete\n");

    ramfs_init();
    
    shell_run();
}


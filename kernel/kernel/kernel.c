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
#include <kernel/multiboot.h>

#include "../arch/i386/idt.h"
#include "../arch/i386/pic.h"
#include "../arch/i386/paging.h"

extern void gdt_set_tss_base(void);
extern uint8_t stack_bottom[];
extern uint8_t stack_top[];
#define MIN_RAM_BYTES (4u * 1024 * 1024)

void kernel_early(void)
{
	terminal_initialize();
}

#define MIN_RAM_KIB 3500u // min useable ram + a small threshhold

static uint32_t print_memory_map(uint32_t magic, const multiboot_info_t *mbi)
{
    if (magic != BOOT_MAGIC || mbi == NULL || !(mbi->flags & MBI_FLAG_MMAP)) {
        printk("[mem] no memory map from bootloader (magic=%x)\n", magic);
        return 0;
    }

    static const char *names[] = { "?", "usable", "reserved", "ACPI reclaim",
                                   "ACPI NVS", "bad" };
    uint32_t total_kib = 0;
    uint32_t p = mbi->mmap_addr, end = p + mbi->mmap_length;

    printk("[mem] BIOS memory map:\n");
    while (p < end) {
        const mmap_entry_t *e = (const mmap_entry_t *)p;
        uint32_t kib = (uint32_t)(e->length >> 10);
        printk("  %08x%08x  %u KiB  %s\n",
               (uint32_t)(e->base >> 32), (uint32_t)e->base, kib,
               e->type < 6 ? names[e->type] : "?");
        if (e->type == 1) total_kib += kib;
        p += e->size + 4;
    }
    printk("[mem] usable: %u KiB\n", total_kib);
    return total_kib;
}

void kernel_main(uint32_t magic, multiboot_info_t *mbi)
{
    uint32_t total_kib = print_memory_map(magic, mbi);
    if (total_kib < MIN_RAM_KIB) {
        printk("[mem] not enough RAM: %u KiB usable, need more than %u KiB.\n",
               total_kib, MIN_RAM_KIB);
        for (;;) asm volatile("cli; hlt");
    }
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

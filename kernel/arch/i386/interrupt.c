#include <stdint.h>
#include <stdio.h>

#include "pic.h"
#include <kernel/portio.h>
#include <kernel/keyboard.h>
#include <kernel/timer.h>
#include <kernel/ramfs.h>
#include <kernel/portio.h>
#include <kernel/tty.h>

struct interrupt_context
{
	uint32_t cr2;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t int_no;
	uint32_t err_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp; /* If (cs & 0x3) != 0 */
	uint32_t ss;  /* If (cs & 0x3) != 0 */
};

void isr_handler(struct interrupt_context* int_ctx)
{
    printk("[EXCEPTION] INT %lu  ERR=%lu  EIP=0x%lx\n",
                    (unsigned long)int_ctx->int_no,
                    (unsigned long)int_ctx->err_code,
                    (unsigned long)int_ctx->eip);

    for(;;); // halt on exception for safety
}

void page_fault_handler(struct interrupt_context* int_ctx)
{
    uint32_t fault_address;
    asm volatile ("mov %%cr2, %0" : "=r"(fault_address));

    printk("[PAGE FAULT] addr=0x%08x err=0x%08x eip=0x%08x\n",
           fault_address,
           int_ctx->err_code,
           int_ctx->eip);

    for(;;); // halt on page fault
}

void irq_handler(struct interrupt_context* int_ctx)
{
    uint8_t irq = int_ctx->int_no - 32;

    // Spurious IRQ handling
    if ( irq == 7 && !(pic_read_isr() & (1 << 7)) )
        return;
    if ( irq == 15 && !(pic_read_isr() & (1 << 15)) )
        return pic_eoi_master();

    if (irq == 1) {
        // Keyboard interrupt
        uint8_t scancode = keyboard_read_scancode();
        if (keyboard_is_key_pressed(scancode)) {
            if (scancode == KEY_BACKSPACE) {
                keyboard_buffer_push('\b');
            } else if (scancode == KEY_ENTER) {
                keyboard_buffer_push('\n');
            } else {
                char ascii = keyboard_scancode_to_ascii(scancode);
                if (ascii) {
                    keyboard_buffer_push(ascii);
                }
            }
        }
    } else if (irq == 0) {
        // Timer interrupt (PIT IRQ 0)
        timer_tick();
    }

    // Send EOI
    if ( irq >= 8 )
        pic_eoi_slave();
    pic_eoi_master();
}


/* Program break for the (currently: single) running user process. Lives
 * within [_user_heap_start, _user_heap_end), a region paging_initialize()
 * marks PTE_USER. NULL means "not yet initialized". Once this kernel grows
 * multiple processes, this needs to move into a per-process struct. */
extern const char _user_heap_start;
extern const char _user_heap_end;
static uint8_t* user_brk = NULL;

static void* sbrk_handler(int increment)
{
    if (user_brk == NULL) {
        user_brk = (uint8_t*)&_user_heap_start;
    }

    if (increment < 0) {
        /* Shrinking isn't supported yet — refuse rather than silently
         * doing the wrong thing. */
        return (void*)-1;
    }

    uint8_t* old_brk = user_brk;
    uint8_t* new_brk = user_brk + increment;

    if (new_brk > (uint8_t*)&_user_heap_end) {
        return (void*)-1; // user heap exhausted
    }

    user_brk = new_brk;
    return old_brk;
}

void syscall_handler(struct interrupt_context* int_ctx)
{
    uint32_t syscall_num = int_ctx->eax;
    uint32_t arg1 = int_ctx->ebx;
    uint32_t arg2 __attribute__((unused)) = int_ctx->ecx;
    uint32_t arg3 __attribute__((unused)) = int_ctx->edx;

    switch (syscall_num) {
        case 0: // exit
            printk("[SYSCALL] exit(%lu)\n", (unsigned long)arg1);
            for(;;); // Halt the system for now
            break;
        case 1: // putchar
            putchar((int)arg1);
            break;
        case 2: // getchar
            if (keyboard_buffer_has_data()) {
                char c = keyboard_buffer_pop();
                int_ctx->eax = (uint32_t)c;
            } else {
                int_ctx->eax = (uint32_t) '\0'; // Return null if no data available
            }
            break;
        case 3: // get_ticks
            int_ctx->eax = timer_get_ticks();
            break;
        case 4: // sbrk
            int_ctx->eax = (uint32_t)sbrk_handler((int)arg1);
            break;
        case 5: // ls(path) — path may be an empty string for root
            ramfs_ls((const char*)arg1);
            int_ctx->eax = 0;
            break;
        case 6: // open(path, flags)
            int_ctx->eax = (uint32_t)ramfs_open((const char*)arg1, (int)arg2);
            break;
        case 7: // close(fd)
            int_ctx->eax = (uint32_t)ramfs_close((int)arg1);
            break;
        case 8: // read(fd, buf, count)
            int_ctx->eax = (uint32_t)ramfs_read((int)arg1, (void*)arg2, (size_t)arg3);
            break;
        case 9: // write(fd, buf, count)
            int_ctx->eax = (uint32_t)ramfs_write((int)arg1, (const void*)arg2, (size_t)arg3);
            break;
        case 10: // mkdir(path)
            int_ctx->eax = (uint32_t)ramfs_mkdir((const char*)arg1);
            break;
        case 11: // unlink(path)
            int_ctx->eax = (uint32_t)ramfs_unlink((const char*)arg1);
            break;
        case 14: // rmdir(path)
            int_ctx->eax = (uint32_t)ramfs_rmdir((const char*)arg1);
            break;
        case 12: // clear_screen
            terminal_initialize();
            int_ctx->eax = 0;
            break;
        case 13:
            while (inport8(0x64) & 0x02) {
                // wait for the controller's input buffer to be clear
            }
            outport8(0x64, 0xFE);
            for (;;) {
                // if the reset didn't take for some reason, just halt
                asm volatile("hlt");
            }
            break;
        default:
            printk("[SYSCALL] unknown syscall %lu\n", (unsigned long)syscall_num);
            break;
    }
}

void interrupt_handler(struct interrupt_context* int_ctx)
{
	if ( int_ctx->int_no == 14 )
		page_fault_handler(int_ctx);
	else if ( int_ctx->int_no < 32 )
		isr_handler(int_ctx);
	else if ( 32 <= int_ctx->int_no && int_ctx->int_no < 32 + 16 )
		irq_handler(int_ctx);
	else if ( int_ctx->int_no == 0x80 )
		syscall_handler(int_ctx);
}

#include <stdint.h>
#include <stdio.h>

#include "pic.h"
#include <kernel/portio.h>
#include <kernel/keyboard.h>
#include <kernel/timer.h>
#include <kernel/ramfs.h>
#include <kernel/portio.h>
#include <kernel/tty.h>
#include <kernel/elf.h>
#include <kernel/usermode.h>
#include <kernel/sched.h>
#include <kernel/exec.h>
#include "../arch/i386/paging.h"
#include <kernel/kmalloc.h>
#include <sys/abi.h>
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
        // schedule();
    }

    // Send EOI
    if ( irq >= 8 )
        pic_eoi_slave();
    pic_eoi_master();
}

extern const char _user_heap_start;
extern const char _user_heap_end;
uint8_t* user_brk = NULL; /* saved/restored by exec_run() */

static void* sbrk_handler(int increment)
{
    if (user_brk == NULL) {
        user_brk = (uint8_t*)&_user_heap_start;
    }

    uint8_t* old_brk = user_brk;
    uint8_t* new_brk = user_brk + increment;

    if (increment < 0 && new_brk < (uint8_t*)&_user_heap_start) {
        return (void*)-1; /* can't shrink below the start of the heap */
    }

    if (new_brk > (uint8_t*)&_user_heap_end) {
        return (void*)-1; // user heap exhausted
    }

    user_brk = new_brk;
    return old_brk;
}


static int sys_read(int fd, void *buf, size_t count)
{
    if (!buf) return -CHALES_EFAULT;
    if (fd == CHALES_FD_STDIN) {

        size_t n = 0;
        char *out = (char *)buf;
        while (n < count && keyboard_buffer_has_data()) {
            out[n++] = keyboard_buffer_pop();
        }
        if (count == 0) return 0;
        return n ? (int)n : -CHALES_EAGAIN;
    }
    if (fd < CHALES_FD_FILE_BASE) return -CHALES_EBADF;
    int r = ramfs_read(fd - CHALES_FD_FILE_BASE, buf, count);
    return r < 0 ? -CHALES_EBADF : r;
}

static int sys_write(int fd, const void *buf, size_t count)
{
    if (!buf) return -CHALES_EFAULT;
    if (fd == CHALES_FD_STDOUT || fd == CHALES_FD_STDERR) {
        terminal_write((const char *)buf, count);
        return (int)count;
    }
    if (fd < CHALES_FD_FILE_BASE) return -CHALES_EBADF;
    int r = ramfs_write(fd - CHALES_FD_FILE_BASE, buf, count);
    return r < 0 ? (r == -CHALES_ENOSPC ? r : -CHALES_EBADF) : r;
}

static int sys_close(int fd)
{
    if (fd >= 0 && fd < CHALES_FD_FILE_BASE) return 0; /* console: nothing to do */
    return ramfs_close(fd - CHALES_FD_FILE_BASE) < 0 ? -CHALES_EBADF : 0;
}

static int sys_wrap_fd(int r)
{
    return r < 0 ? r : r + CHALES_FD_FILE_BASE;
}

static int sys_lseek(int fd, int32_t off, int whence)
{
    if (fd >= 0 && fd < CHALES_FD_FILE_BASE) return -CHALES_ESPIPE;
    return ramfs_lseek(fd - CHALES_FD_FILE_BASE, off, whence);
}

static int sys_fstat(int fd, struct chales_stat *st)
{
    if (!st) return -CHALES_EFAULT;
    if (fd >= 0 && fd < CHALES_FD_FILE_BASE) {
        st->st_mode = CHALES_S_IFCHR | 0620;
        st->st_size = 0;
        st->st_ino = fd;
        st->st_nlink = 1;
        return 0;
    }
    return ramfs_fstat(fd - CHALES_FD_FILE_BASE, st);
}

void syscall_handler(struct interrupt_context* int_ctx)
{
    uint32_t syscall_num = int_ctx->eax;
    uint32_t arg1 = int_ctx->ebx;
    uint32_t arg2 __attribute__((unused)) = int_ctx->ecx;
    uint32_t arg3 __attribute__((unused)) = int_ctx->edx;

    switch (syscall_num) {
        case 0: // exit(code)
            sched_task_exit(current_task, ((int)arg1 & 0xFF) << 8);
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
            /* legacy flags: bit 0 = create. Returns -1 on failure. */
            {
                int r = ramfs_open((const char*)arg1, (int)arg2);
                int_ctx->eax = (uint32_t)(r < 0 ? -1 : sys_wrap_fd(r));
            }
            break;
        case 7: // close(fd)
            int_ctx->eax = (uint32_t)sys_close((int)arg1);
            break;
        case 8: // read(fd, buf, count)
            int_ctx->eax = (uint32_t)sys_read((int)arg1, (void*)arg2, (size_t)arg3);
            break;
        case 9: // write(fd, buf, count)
            int_ctx->eax = (uint32_t)sys_write((int)arg1, (const void*)arg2, (size_t)arg3);
            break;
        case 10: // mkdir(path)
            int_ctx->eax = (uint32_t)ramfs_mkdir((const char*)arg1);
            break;
        case 11: // unlink(path)
            int_ctx->eax = (uint32_t)ramfs_unlink((const char*)arg1);
            break;
        case 12: // rmdir(path)
            int_ctx->eax = (uint32_t)ramfs_rmdir((const char*)arg1);
            break;
        case 13: // clear_screen
            terminal_initialize();
            int_ctx->eax = 0;
            break;
        case 14: // outport8
            int_ctx->eax = outport8((uint16_t)arg1, (uint8_t)arg2);
            break;
        case 15: // outport16
            int_ctx->eax = outport16((uint16_t)arg1, (uint16_t)arg2);
            break;
        case 16: // outport32
            int_ctx->eax = outport32((uint16_t)arg1, (uint32_t)arg2);
            break;
        case 17: // inport8
            int_ctx->eax = inport8((uint16_t)arg1);
            break;
        case 18: // inport16
            int_ctx->eax = inport16((uint16_t)arg1);
            break;
        case 19: // inport32
            int_ctx->eax = inport32((uint16_t)arg1);
            break;
        case 20: // reset
            while (inport8(0x64) & 0x02) {
                // wait for the controller's input buffer to be clear
            }
            outport8(0x64, 0xFE);
            for (;;) {
                // if the reset didn't take for some reason, just halt
                asm volatile("hlt");
            }
            break;
        case 21: // execv(path, argv, envp): runs the program to completion
            int_ctx->eax = (uint32_t)exec_run((const char*)arg1, (char **)arg2, (char **)arg3);
            break;
        case SYS_wait: { // wait(int *status): reap the last child that finished
            task_t *me = current_task;
            if (!me->zombie_valid) {
                int_ctx->eax = (uint32_t)-CHALES_ECHILD;
            } else {
                if (arg1) *(int *)arg1 = me->zombie_status;
                int_ctx->eax = me->zombie_pid;
                me->zombie_valid = 0;
            }
            break;
        }

        case SYS_open2: // open(path, newlib flags, mode)
            int_ctx->eax = (uint32_t)sys_wrap_fd(ramfs_open2((const char*)arg1, (int)arg2));
            break;
        case SYS_lseek:
            int_ctx->eax = (uint32_t)sys_lseek((int)arg1, (int32_t)arg2, (int)arg3);
            break;
        case SYS_fstat:
            int_ctx->eax = (uint32_t)sys_fstat((int)arg1, (struct chales_stat*)arg2);
            break;
        case SYS_stat:
            if (!arg1 || !arg2) {
                int_ctx->eax = (uint32_t)-CHALES_EFAULT;
            } else {
                int_ctx->eax = (uint32_t)ramfs_stat((const char*)arg1, (struct chales_stat*)arg2);
            }
            break;
        case SYS_isatty:
            int_ctx->eax = (arg1 < CHALES_FD_FILE_BASE) ? 1 : 0;
            break;
        case SYS_getpid:
            int_ctx->eax = current_task->pid;
            break;
        case SYS_kill: {
            uint32_t sig = arg2;
            task_t *target = sched_find_task(arg1);
            if (sig > 64) {
                int_ctx->eax = (uint32_t)-CHALES_EINVAL;
            } else if (!target) {
                int_ctx->eax = (uint32_t)-CHALES_ESRCH;
            } else if (sig == 0) {
                int_ctx->eax = 0; /* existence probe */
            } else {
                int_ctx->eax = 0;
                /* No signal delivery yet: any signal terminates the task. */
                sched_task_exit(target, (int)sig);
            }
            break;
        }
        case SYS_times: {
            struct chales_tms *t = (struct chales_tms*)arg1;
            uint32_t ticks = timer_get_ticks();
            if (t) {
                /* No user/kernel accounting: all time is reported as user time. */
                t->tms_utime = ticks;
                t->tms_stime = 0;
                t->tms_cutime = 0;
                t->tms_cstime = 0;
            }
            int_ctx->eax = ticks;
            break;
        }
        case SYS_gettimeofday: {
            struct chales_timeval *tv = (struct chales_timeval*)arg1;
            uint32_t ms = timer_get_ms();
            if (!tv) {
                int_ctx->eax = (uint32_t)-CHALES_EFAULT;
            } else {
                // No RTC support yet: this is time since boot.
                tv->tv_sec = ms / 1000;
                tv->tv_usec = (ms % 1000) * 1000;
                int_ctx->eax = 0;
            }
            break;
        }

        default:
            int_ctx->eax = (uint32_t)-CHALES_ENOSYS;
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

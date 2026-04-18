#include <stdint.h>
#include <stdio.h>

#include "pic.h"
#include <kernel/portio.h>
#include <kernel/keyboard.h>
#include <kernel/timer.h>

struct interrupt_context
{
	uint32_t cr2;
	uint32_t gs;
	uint32_t fs;
	uint32_t ds;
	uint32_t es;
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
    printf("[EXCEPTION] INT %lu  ERR=%lu  EIP=0x%lx\n",
                    (unsigned long)int_ctx->int_no,
                    (unsigned long)int_ctx->err_code,
                    (unsigned long)int_ctx->eip);

    for(;;); // halt on exception for safety
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


void interrupt_handler(struct interrupt_context* int_ctx)
{
	if ( int_ctx->int_no < 32 )
		isr_handler(int_ctx);
	else if ( 32 <= int_ctx->int_no && int_ctx->int_no < 32 + 16 )
		irq_handler(int_ctx);
}
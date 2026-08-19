#ifndef _MYOS_ARCH_I386_USERMODE_H
#define _MYOS_ARCH_I386_USERMODE_H

void enter_user_mode(void);
// made for elf loading
void enter_user_mode_at(uint32_t entry_point, uint32_t stack_top);

#endif

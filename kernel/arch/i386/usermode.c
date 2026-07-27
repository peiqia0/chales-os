#include <stdint.h>
#include <stdio.h>

// User mode stack - allocated in the dedicated user section so ring 3 can use it.
static uint32_t user_stack[1024] __attribute__((aligned(16), section(".user")));

extern void user_main(void);

void enter_user_mode(void)
{
    uint32_t user_stack_top = (uint32_t)&user_stack[1024];
    
    printk("[KERNEL] Entering Ring 3 user mode...\n");
    printk("[KERNEL] User stack top: 0x%lx\n", (unsigned long)user_stack_top);
    printk("[KERNEL] User entry point: 0x%lx\n", (unsigned long)&user_main);
    
    // Construct an IRET frame to switch to Ring 3.
    // The IRET instruction pops: EIP, CS, EFLAGS, [ESP, SS if privilege level changed] 
    // We push in reverse order: SS, ESP, EFLAGS, CS, EIP
    // User CS selector: 0x1B (GDT index 3, RPL=3)
    // User DS selector: 0x23 (GDT index 4, RPL=3)
    
    asm volatile (
        // Set user data segments before entering user mode
        "movw $0x23, %%ax\n\t"      // User Data Segment
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"
        
        // Prepare IRET frame on current stack
        "movl $0x23, %%eax\n\t" // User Data Segment (SS) 
        "pushl %%eax\n\t"
        "pushl %0\n\t" // User stack ESP 
        "pushfl\n\t" // Current EFLAGS 
        "popl %%eax\n\t"
        "orl $0x200, %%eax\n\t" // Enable interrupts (IF flag) 
        "pushl %%eax\n\t"
        "movl $0x1B, %%eax\n\t" // User Code Segment (CS)
        "pushl %%eax\n\t"
        "pushl %1\n\t" // User entry point (EIP)
        
        // Jump to user mode via IRET
        "iret\n\t"
        : 
        : "r"(user_stack_top), "r"(&user_main)
        : "eax"
    );
}

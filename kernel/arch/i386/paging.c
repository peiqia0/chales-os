#include <stdint.h>
#include <stddef.h>
#include "paging.h"

/* Simple 4KB paging: one page directory and one page table covering 0..4MiB
 * We identity-map the first 4MiB, marking only the dedicated .user section
 * user-accessible while keeping kernel pages supervisor-only.
 */

/* PDE/PTE flags */
#define PTE_P 0x1
#define PTE_RW 0x2
#define PTE_USER 0x4

/* Align page structures to 4KiB */
static uint32_t page_directory[1024] __attribute__((aligned(4096)));
static uint32_t page_table0[1024] __attribute__((aligned(4096)));

extern const char _user_start;
extern const char _user_end;
extern const char _user_heap_start;
extern const char _user_heap_end;

static inline uint32_t page_index(uint32_t addr)
{
    return addr >> 12;
}

void paging_initialize(void)
{
    const uint32_t user_start = (uint32_t)(uintptr_t)&_user_start;
    const uint32_t user_end = (uint32_t)(uintptr_t)&_user_end;
    const uint32_t user_heap_start = (uint32_t)(uintptr_t)&_user_heap_start;
    const uint32_t user_heap_end = (uint32_t)(uintptr_t)&_user_heap_end;

    /* Zero structures */
    for (size_t i = 0; i < 1024; i++) {
        page_directory[i] = 0;
        page_table0[i] = 0;
    }

    /* Fill page_table0: identity map 0..4MiB with supervisor-only pages by default. */
    for (size_t i = 0; i < 1024; i++) {
        uint32_t phys = (uint32_t)(i * 0x1000);
        page_table0[i] = phys | PTE_P | PTE_RW;
    }

    /* Make VGA page supervisor-only explicitly. */
    const size_t vga_index = 0xB8000 / 0x1000;
    if (vga_index < 1024) {
        uint32_t phys = (uint32_t)(vga_index * 0x1000);
        page_table0[vga_index] = phys | PTE_P | PTE_RW;
    }

    /* Mark the .user section user-accessible. */
    if (user_end > user_start) {
        uint32_t page = page_index(user_start);
        uint32_t last_page = page_index(user_end - 1);
        for (; page <= last_page; page++) {
            uint32_t phys = (uint32_t)(page * 0x1000);
            page_table0[page] = phys | PTE_P | PTE_RW | PTE_USER;
        }
    }

    if (user_heap_end > user_heap_start) {
        uint32_t page = page_index(user_heap_start);
        uint32_t last_page = page_index(user_heap_end - 1);
        for (; page <= last_page; page++) {
            uint32_t phys = (uint32_t)(page * 0x1000);
            page_table0[page] = phys | PTE_P | PTE_RW | PTE_USER;
        }
    }

    /* Point first PDE to our first page table. Mark PDE present, rw, user so
     * page-level user access is governed by PTE user bits.
     */
    uint32_t pt_phys = (uint32_t)((uintptr_t)page_table0);
    page_directory[0] = pt_phys | PTE_P | PTE_RW | PTE_USER;
}
void paging_map_user_range(uint32_t start_addr, uint32_t end_addr)
{
    if (end_addr <= start_addr) {
        return;
    }

    uint32_t page = page_index(start_addr);
    uint32_t last_page = page_index(end_addr - 1);

    if (last_page >= 1024) {
        last_page = 1023;
    }

    for (; page <= last_page; page++) {
        uint32_t phys = (uint32_t)(page * 0x1000);
        page_table0[page] = phys | PTE_P | PTE_RW | PTE_USER;
    }
}
void paging_enable(void)
{
    uint32_t pd_phys = (uint32_t)((uintptr_t)page_directory);
    asm volatile (
        "movl %0, %%cr3\n\t"
        "movl %%cr0, %%eax\n\t"
        "orl $0x80000000, %%eax\n\t"
        "movl %%eax, %%cr0\n\t"
        "jmp 1f\n\t"
        "1:\n\t"
        :: "r"(pd_phys)
        : "eax", "memory");
}

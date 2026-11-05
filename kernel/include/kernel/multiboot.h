#ifndef _KERNEL_MULTIBOOT_H
#define _KERNEL_MULTIBOOT_H
#include <stdint.h>

#define BOOT_MAGIC        0x2BADB002
#define MBI_FLAG_MMAP     (1u << 6)

typedef struct {
    uint32_t flags;
    uint32_t mem_lower, mem_upper;
    uint32_t boot_device, cmdline;
    uint32_t mods_count, mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;      // offset 44
    uint32_t mmap_addr;        // offset 48
} __attribute__((packed)) multiboot_info_t;

typedef struct {
    uint32_t size;             // excludes this field
    uint64_t base;
    uint64_t length;
    uint32_t type;             // 1 = usable
} __attribute__((packed)) mmap_entry_t;

#endif

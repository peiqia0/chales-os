#include <kernel/elf.h>
#include <kernel/ramfs.h>
#include <kernel/usermode.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "paging.h"

// Bounds of the kernel's own image
// a loaded segment is refused if it would overlap this range
extern const char _kernel_start;
extern const char _kernel_end;

// Everything below 4MiB is identity-mapped (see paging.c)
// nothing else is currently mapped
#define ELF_LOAD_LIMIT 0x400000u

static int elf_validate(const elf32_ehdr_t *eh, size_t size)
{
    if (size < sizeof(elf32_ehdr_t)) {
        printk("[elf] file too small to hold an ELF header\n");
        return -1;
    }

    if (eh->e_ident[EI_MAG0] != ELFMAG0 || eh->e_ident[EI_MAG1] != ELFMAG1 ||
        eh->e_ident[EI_MAG2] != ELFMAG2 || eh->e_ident[EI_MAG3] != ELFMAG3) {
        printk("[elf] bad magic\n");
        return -1;
    }

    if (eh->e_ident[EI_CLASS] != ELFCLASS32) {
        printk("[elf] not a 32-bit ELF (ELFCLASS32)\n");
        return -1;
    }

    if (eh->e_ident[EI_DATA] != ELFDATA2LSB) {
        printk("[elf] not little-endian (ELFDATA2LSB)\n");
        return -1;
    }

    if (eh->e_type != ET_EXEC) {
        printk("[elf] only statically-linked ET_EXEC binaries are supported\n");
        return -1;
    }

    if (eh->e_machine != EM_386) {
        printk("[elf] not an i386 (EM_386) binary\n");
        return -1;
    }

    if (eh->e_phoff == 0 || eh->e_phnum == 0) {
        printk("[elf] no program headers\n");
        return -1;
    }

    // Bounds-check the program header table itself before we walk it.
    if (eh->e_phentsize < sizeof(elf32_phdr_t)) {
        printk("[elf] program header entry too small\n");
        return -1;
    }

    uint64_t phtab_end = (uint64_t)eh->e_phoff +
                          (uint64_t)eh->e_phnum * eh->e_phentsize;
    if (phtab_end > size) {
        printk("[elf] program header table runs past end of file\n");
        return -1;
    }

    return 0;
}

int elf_load(const void *image, size_t size, uint32_t *entry_out)
{
    const uint8_t *bytes = (const uint8_t *)image;
    const elf32_ehdr_t *eh = (const elf32_ehdr_t *)image;

    if (elf_validate(eh, size) != 0) {
        return -1;
    }

    const uint32_t kernel_start = (uint32_t)(uintptr_t)&_kernel_start;
    const uint32_t kernel_end = (uint32_t)(uintptr_t)&_kernel_end;

    // First pass: validate every PT_LOAD segment before touching any memory
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const elf32_phdr_t *ph = (const elf32_phdr_t *)
            (bytes + eh->e_phoff + (uint32_t)i * eh->e_phentsize);

        if (ph->p_type != PT_LOAD) {
            continue;
        }

        if (ph->p_filesz > ph->p_memsz) {
            printk("[elf] segment %u: filesz > memsz\n", (unsigned)i);
            return -1;
        }

        if ((uint64_t)ph->p_offset + ph->p_filesz > size) {
            printk("[elf] segment %u: file range out of bounds\n", (unsigned)i);
            return -1;
        }

        uint32_t seg_start = ph->p_vaddr;
        uint64_t seg_end64 = (uint64_t)ph->p_vaddr + ph->p_memsz; // exclusive

        if (seg_end64 > ELF_LOAD_LIMIT) {
            printk("[elf] segment %u: vaddr range extends past identity-mapped "
                   "memory (limit 0x%lx)\n", (unsigned)i, (unsigned long)ELF_LOAD_LIMIT);
            return -1;
        }

        uint32_t seg_end = (uint32_t)seg_end64;

        if (seg_start < kernel_end && seg_end > kernel_start) {
            printk("[elf] segment %u: vaddr 0x%lx..0x%lx overlaps the kernel "
                   "image (0x%lx..0x%lx)\n", (unsigned)i,
                   (unsigned long)seg_start, (unsigned long)seg_end,
                   (unsigned long)kernel_start, (unsigned long)kernel_end);
            return -1;
        }
    }

    if (eh->e_entry >= ELF_LOAD_LIMIT ||
        (eh->e_entry < kernel_end && eh->e_entry >= kernel_start)) {
        printk("[elf] entry point 0x%lx is outside any valid segment\n",
               (unsigned long)eh->e_entry);
        return -1;
    }

    // Second pass
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const elf32_phdr_t *ph = (const elf32_phdr_t *)
            (bytes + eh->e_phoff + (uint32_t)i * eh->e_phentsize);

        if (ph->p_type != PT_LOAD) {
            continue;
        }

        uint8_t *dst = (uint8_t *)(uintptr_t)ph->p_vaddr;

        if (ph->p_filesz > 0) {
            memcpy(dst, bytes + ph->p_offset, ph->p_filesz);
        }
        if (ph->p_memsz > ph->p_filesz) {
            // .bss-style tail: zero it rather than trusting the file
            memset(dst + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
        }

        paging_map_user_range(ph->p_vaddr, ph->p_vaddr + ph->p_memsz);
    }

    if (entry_out) {
        *entry_out = eh->e_entry;
    }

    return 0;
}

// Dedicated stack for ELF-loaded programs, in the .user section so paging already marks it user-accessible
#define ELF_USER_STACK_WORDS (4096 / sizeof(uint32_t))
static uint32_t elf_user_stack[ELF_USER_STACK_WORDS]
    __attribute__((aligned(16), section(".user")));

int elf_exec_file(const char *path)
{
    int fd = ramfs_open(path, 0);
    if (fd < 0) {
        printk("[elf] cannot open '%s'\n", path);
        return -1;
    }

    // ramfs files can't exceed RAMFS_MAX_FILESIZE
    // read the whole thing into a static buffer so we don't blow the kernel stack
    static uint8_t image[RAMFS_MAX_FILESIZE];
    size_t total = 0;
    int n;
    while (total < sizeof(image) &&
           (n = ramfs_read(fd, image + total, sizeof(image) - total)) > 0) {
        total += (size_t)n;
    }
    ramfs_close(fd);

    if (total == 0) {
        printk("[elf] '%s' is empty or unreadable\n", path);
        return -1;
    }

    uint32_t entry;
    if (elf_load(image, total, &entry) != 0) {
        printk("[elf] failed to load '%s'\n", path);
        return -1;
    }

    uint32_t stack_top = (uint32_t)(uintptr_t)&elf_user_stack[ELF_USER_STACK_WORDS];

    printk("[elf] executing '%s' (entry=0x%lx)\n", path, (unsigned long)entry);
    enter_user_mode_at(entry, stack_top); // does not return

    return 0;
}

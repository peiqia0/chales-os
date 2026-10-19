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

static int count_strings(char **arr)
{
    if (arr == NULL) return 0;
    int count = 0;
    while (arr[count] != NULL) {
        count++;
    }
    return count;
}

static uint32_t strings_total_size(char **arr, int count)
{
    uint32_t total = 0;
    for (int i = 0; i < count; i++) {
        total += strlen(arr[i]) + 1;
    }
    return total;
}
uint32_t elf_build_user_stack(uint32_t stack_base, uint32_t stack_top, char **argv, char **envp) {
    int argc = count_strings(argv);
    int envc = count_strings(envp);

    uint32_t total = strings_total_size(argv, argc) + strings_total_size(envp, envc);

    total += (argc + 1) * sizeof(char*);   // argv[] array + NULL terminator
    total += (envc + 1) * sizeof(char*);   // envp[] array + NULL terminator
    total += sizeof(uint32_t);              // argc itself

    if (total > (stack_top - stack_base)) {
        return 0;  // doesn't fit
    }

    uint32_t cursor = stack_top;

    // Copy string data first
    char *argv_copied[argc];
    char *envp_copied[envc];

    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]) + 1;
        cursor -= len;
        memcpy((void *)(uintptr_t)cursor, argv[i], len);
        argv_copied[i] = (char *)(uintptr_t)cursor;
    }

    for (int i = 0; i < envc; i++) {
        size_t len = strlen(envp[i]) + 1;
        cursor -= len;
        memcpy((void *)(uintptr_t)cursor, envp[i], len);
        envp_copied[i] = (char *)(uintptr_t)cursor;
    }

    // Align down to 4 bytes before writing pointer-sized data.
    cursor &= ~((uint32_t)sizeof(uint32_t) - 1);

    cursor -= sizeof(char*);
    *(char **)(uintptr_t)cursor = NULL;
    for (int i = envc - 1; i >= 0; i--) {
        cursor -= sizeof(char*);
        *(char **)(uintptr_t)cursor = envp_copied[i];
    }
    uint32_t envp_addr = cursor;

    // argv[] array, NULL-terminated, same pattern.
    cursor -= sizeof(char*);
    *(char **)(uintptr_t)cursor = NULL;
    for (int i = argc - 1; i >= 0; i--) {
        cursor -= sizeof(char*);
        *(char **)(uintptr_t)cursor = argv_copied[i];
    }
    uint32_t argv_addr = cursor;

    // Finally, argc itself — this is what crt0's _start pops first.
    cursor -= sizeof(uint32_t);
    *(uint32_t *)(uintptr_t)cursor = (uint32_t)argc;

    return cursor;
}

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

int elf_extent(const void *image, size_t size, uint32_t *lo_out, uint32_t *hi_out)
{
    const uint8_t *bytes = (const uint8_t *)image;
    const elf32_ehdr_t *eh = (const elf32_ehdr_t *)image;

    if (elf_validate(eh, size) != 0) {
        return -1;
    }

    uint32_t lo = 0xFFFFFFFFu, hi = 0;
    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const elf32_phdr_t *ph = (const elf32_phdr_t *)
            (bytes + eh->e_phoff + (uint32_t)i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD) {
            continue;
        }
        uint64_t end = (uint64_t)ph->p_vaddr + ph->p_memsz;
        if (end > ELF_LOAD_LIMIT) {
            return -1;
        }
        if (ph->p_vaddr < lo) lo = ph->p_vaddr;
        if ((uint32_t)end > hi) hi = (uint32_t)end;
    }
    if (hi == 0) {
        return -1;
    }
    *lo_out = lo;
    *hi_out = hi;
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
    size_t total = 0;
    const uint8_t *image = ramfs_file_data(path, &total);
    if (image == NULL) {
        printk("[elf] cannot open '%s'\n", path);
        return -1;
    }

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

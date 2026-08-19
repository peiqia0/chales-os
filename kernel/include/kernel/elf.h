#ifndef _KERNEL_ELF_H
#define _KERNEL_ELF_H

#include <stdint.h>
#include <stddef.h>

// Minimal ELF32 structures, per the System V ABI.

typedef uint16_t Elf32_Half;   // Unsigned half int
typedef uint32_t Elf32_Off;    // Unsigned offset
typedef uint32_t Elf32_Addr;   // Unsigned address
typedef uint32_t Elf32_Word;   // Unsigned int
typedef int32_t  Elf32_Sword;  // Signed int

#define EI_NIDENT 16

typedef struct {
    uint8_t    e_ident[EI_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off  e_phoff;
    Elf32_Off  e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} elf32_ehdr_t;

typedef struct {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} elf32_phdr_t;

// e_ident[] indices
#define EI_MAG0    0
#define EI_MAG1    1
#define EI_MAG2    2
#define EI_MAG3    3
#define EI_CLASS   4
#define EI_DATA    5
#define EI_VERSION 6

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define ELFCLASS32  1
#define ELFDATA2LSB 1

// e_type
#define ET_EXEC 2

// e_machine
#define EM_386 3

// p_type
#define PT_LOAD 1

// p_flags
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

// Loads a validated ELF32 executable image already in memory
int elf_load(const void *image, size_t size, uint32_t *entry_out);

// Convenience wrapper: reads `path` from ramfs, validates + loads it with elf_load()
int elf_exec_file(const char *path);

#endif

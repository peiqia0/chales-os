# chales-os stage 1: the boot sector.

# The BIOS loads this to 0x7C00 in real mode with the boot drive in DL.
# All it does is read stage 2 (the next sectors on the disk) to 0x7E00 and jump to it.

.include "layout.inc"
.intel_syntax noprefix
.code16

.section .text
.global _start
_start:
 # Some BIOSes enter at 07C0:0000 rather than 0000:7C00; normalise CS.
    .byte 0xEA
    .word reload_cs, 0
reload_cs:
    jmp skip_table
 # Bytes 8..63 are the El Torito boot info table (patched in by xorriso).
.org 64
skip_table:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00 # stack grows down from just below this code
    sti
    cld

    mov [boot_drive], dl

 # Booted from CD (no emulation)? The BIOS already loaded stage 2 for us,
 # and a CD can't be read in 512-byte units anyway.
    cmp dword ptr [BOOT_INFO_FILE_LBA], 0
    jne run_stage2

    mov si, offset dap
    mov ah, 0x42 # INT 13h extended read
    int 0x13
    jc disk_error

run_stage2:
    mov dl, [boot_drive] # stage 2 gets the boot drive in DL, too
    .byte 0xEA # jmp 0x0000:STAGE2_ADDR
    .word STAGE2_ADDR, 0

disk_error:
    mov si, offset msg_error
print:
    lodsb
    test al, al
    jz halt
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print
halt:
    cli
    hlt
    jmp halt

msg_error: .asciz "chales-os: stage 1 disk error"

# Disk address packet for INT 13h AH=42h.
dap:
    .byte 0x10, 0
    .word STAGE2_SECTORS
    .word STAGE2_ADDR, 0
    .long STAGE2_LBA, 0

boot_drive: .byte 0

.org 510
.word 0xAA55

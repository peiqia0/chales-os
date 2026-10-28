# chales-os stage 2.

.include "layout.inc"
.intel_syntax noprefix
.code16

.equ MAX_PHDRS, 16

.section .text
.global _start
_start:
    mov [boot_drive], dl
    xor ax, ax
    mov ds, ax
    mov es, ax
    cld

    mov si, offset msg_banner
    call puts

 # Work out the sector size we are reading in: 512-byte sectors on a disk,
 # 2048-byte sectors when booted from a CD (see layout.inc).
    mov dword ptr [kernel_base], KERNEL_LBA
    mov byte ptr [sect_shift], 9
    mov dword ptr [unit_mask], 511
    mov dword ptr [bounce_units], BOUNCE_SECTORS
    mov dword ptr [hdr_units], 8
    mov eax, [BOOT_INFO_FILE_LBA]
    test eax, eax
    jz sector_size_known
    add eax, KERNEL_CD_OFFSET
    mov [kernel_base], eax
    mov byte ptr [sect_shift], 11
    mov dword ptr [unit_mask], 2047
    mov dword ptr [bounce_units], BOUNCE_CD_SECTORS
    mov dword ptr [hdr_units], 2
sector_size_known:

    call enable_a20
    call enter_unreal

 # ---- read the ELF header and program headers (first 4 KiB) ----
    mov eax, [kernel_base]
    mov ecx, [hdr_units]
    call read_sectors

    mov esi, BOUNCE_ADDR
    cmp dword ptr [esi], 0x464C457F # "\x7fELF"
    jne bad_kernel
    cmp byte ptr [esi + 4], 1 # ELFCLASS32
    jne bad_kernel
    cmp word ptr [esi + 42], 32 # e_phentsize
    jne bad_kernel
    movzx ecx, word ptr [esi + 44] # e_phnum
    test ecx, ecx
    jz bad_kernel
    cmp ecx, MAX_PHDRS
    ja bad_kernel
    mov [phnum], ecx
    mov eax, [esi + 24] # e_entry
    mov [entry], eax
    mov eax, [esi + 28] # e_phoff
    mov edx, ecx
    shl edx, 5
    add edx, eax
    cmp edx, 4096 # table must lie inside what we read
    ja bad_kernel

 # Keep a private copy: the bounce buffer is reused for segment data.
    add esi, eax
    mov edi, offset phbuf
    shl ecx, 5
    rep movs byte ptr es:[edi], byte ptr ds:[esi]

 # ---- load every PT_LOAD segment ----
    mov ebx, offset phbuf
next_segment:
    cmp dword ptr [phnum], 0
    je loaded
    cmp dword ptr [ebx], 1 # PT_LOAD
    jne skip_segment
    mov eax, [ebx + 4] # p_offset
    mov [seg_off], eax
    mov eax, [ebx + 12] # p_paddr
    mov [seg_dest], eax
    mov eax, [ebx + 16] # p_filesz
    mov [seg_filesz], eax
    mov eax, [ebx + 20] # p_memsz
    mov [seg_memsz], eax
    call load_segment
skip_segment:
    add ebx, 32
    dec dword ptr [phnum]
    jmp next_segment

loaded:
    mov si, offset msg_jump
    call puts

 # ---- into 32-bit protected mode, then the kernel ----
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    .byte 0x66, 0xEA # jmp 0x08:pm_entry (32-bit offset)
    .long pm_entry
    .word 0x08

.code32
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x7C00
    mov eax, 0x2BADB002
    xor ebx, ebx
    mov edx, [entry]
    jmp edx

.code16


# load_segment: copy [seg_off, seg_off + seg_filesz) of the kernel file to
# physical address seg_dest and zero the rest of seg_memsz (the .bss tail).

load_segment:
    pushad

    mov edi, [seg_dest]
    cmp edi, 0x100000 # never overwrite the BIOS area/loader
    jb bad_kernel
    mov ecx, [seg_memsz]
    mov eax, edi
    add eax, ecx
    jc bad_kernel
    xor eax, eax
    rep stos byte ptr es:[edi], al # zero the whole segment first
    mov edi, [seg_dest]

chunk:
    mov ecx, [seg_filesz] # bytes still to copy
    test ecx, ecx
    jz segment_done

    mov eax, [seg_off]
    mov edx, eax
    and edx, [unit_mask] # offset within the first sector
    mov cl, [sect_shift]
    shr eax, cl
    add eax, [kernel_base] # first sector to read

    mov ebx, [seg_filesz]
    add ebx, edx
    add ebx, [unit_mask]
    shr ebx, cl # sectors needed for what's left
    cmp ebx, [bounce_units]
    jbe count_ok
    mov ebx, [bounce_units]
count_ok:
    push edx
    push ebx
    mov ecx, ebx
    call read_sectors
    pop ebx
    pop edx

    mov cl, [sect_shift]
    shl ebx, cl
    sub ebx, edx # usable bytes in the buffer
    mov ecx, [seg_filesz]
    cmp ecx, ebx
    jbe size_ok
    mov ecx, ebx
size_ok:
    mov esi, BOUNCE_ADDR
    add esi, edx
    mov ebx, ecx # bytes copied this round
    rep movs byte ptr es:[edi], byte ptr ds:[esi]
    add [seg_off], ebx
    sub [seg_filesz], ebx
    jmp chunk

segment_done:
    popad
    ret


# read_sectors: EAX = first LBA, ECX = sector count (<= BOUNCE_SECTORS).
# Reads into the bounce buffer. Preserves all registers.

read_sectors:
    pushad
    mov [dap_lba], eax
    mov [want_count], cx
    mov word ptr [dap_off], 0
    mov byte ptr [retries], 3
read_try:
    mov ax, [want_count]
    mov [dap_count], ax
    mov si, offset dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jnc read_ok
    mov [err_ah], ah
    xor ah, ah # reset the controller and retry
    mov dl, [boot_drive]
    int 0x13
    dec byte ptr [retries]
    jnz read_try

 # Multi-sector read failed: some BIOSes reject it, so go one sector at a
 # time into consecutive parts of the bounce buffer.
    mov word ptr [dap_off], 0
one_loop:
    mov byte ptr [retries], 3
one_try:
    mov word ptr [dap_count], 1
    mov si, offset dap
    mov dl, [boot_drive]
    mov ah, 0x42
    int 0x13
    jnc one_ok
    mov [err_ah], ah
    xor ah, ah
    mov dl, [boot_drive]
    int 0x13
    dec byte ptr [retries]
    jnz one_try
    jmp disk_error
one_ok:
    inc dword ptr [dap_lba]
    mov cl, [sect_shift]
    mov ax, 1
    shl ax, cl
    add [dap_off], ax
    dec word ptr [want_count]
    jnz one_loop
    mov word ptr [dap_off], 0
    jmp read_ok
read_ok:
    popad
    call enter_unreal # some BIOSes drop the 4 GiB limits
    ret

enter_unreal:
    pushad
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp unreal_flush
unreal_flush:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov eax, cr0
    and al, 0xFE
    mov cr0, eax
    xor ax, ax
    mov ds, ax
    mov es, ax
    sti
    popad
    ret

# A20
enable_a20:
    call check_a20
    jnz a20_done
    in al, 0x92
    or al, 2
    and al, 0xFE # never write the reset bit
    out 0x92, al
    call check_a20
    jnz a20_done

    call kbc_wait_in
    mov al, 0xAD
    out 0x64, al
    call kbc_wait_in
    mov al, 0xD0
    out 0x64, al
    call kbc_wait_out
    in al, 0x60
    push ax
    call kbc_wait_in
    mov al, 0xD1
    out 0x64, al
    call kbc_wait_in
    pop ax
    or al, 2
    out 0x60, al
    call kbc_wait_in
    mov al, 0xAE
    out 0x64, al
    call kbc_wait_in
    call check_a20
    jnz a20_done
    mov si, offset msg_a20
    jmp fatal
a20_done:
    ret

kbc_wait_in:
    in al, 0x64
    test al, 2
    jnz kbc_wait_in
    ret
kbc_wait_out:
    in al, 0x64
    test al, 1
    jz kbc_wait_out
    ret

check_a20:
    pushf
    push ds
    push es
    push si
    push di
    xor ax, ax
    mov ds, ax
    not ax
    mov es, ax
    mov si, 0x0500
    mov di, 0x0510
    mov al, [si]
    push ax
    mov al, es:[di]
    push ax
    mov byte ptr [si], 0x00
    mov byte ptr es:[di], 0xFF
    cmp byte ptr [si], 0xFF
    mov bl, 0
    je a20_wrapped
    mov bl, 1
a20_wrapped:
    pop ax
    mov es:[di], al
    pop ax
    mov [si], al
    pop di
    pop si
    pop es
    pop ds
    popf
    test bl, bl
    ret


# Text output and errors

puts:
    lodsb
    test al, al
    jz puts_done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp puts
puts_done:
    ret

disk_error:
    mov si, offset msg_disk
    call puts
    mov si, offset msg_ah
    call puts
    mov al, [err_ah]
    call hex8
    mov si, offset msg_lba
    call puts
    mov eax, [dap_lba]
    call hex32
    mov si, offset msg_cnt
    call puts
    mov al, [boot_drive]
    call hex8
    jmp hang

hex32:
    push eax
    shr eax, 16
    call hex16
    pop eax
hex16:
    push ax
    mov al, ah
    call hex8
    pop ax
hex8:
    push ax
    shr al, 4
    call hex4
    pop ax
    and al, 15
hex4:
    cmp al, 10
    sbb al, 0x69
    das
    mov ah, 0x0E
    xor bx, bx
    int 0x10
    ret
bad_kernel:
    mov si, offset msg_kernel
fatal:
    call puts
hang:
    cli
    hlt
    jmp hang


# Data

msg_banner: .asciz "chales-os bootloader\r\nLoading kernel... "
msg_jump:   .asciz "ok\r\n"
msg_disk:   .asciz "\r\ndisk read error"
msg_ah:     .asciz " ah="
msg_lba:    .asciz " lba="
msg_cnt:    .asciz " drive="
msg_kernel: .asciz "\r\nkernel image is not a loadable ELF32"
msg_a20:    .asciz "\r\ncannot enable the A20 line"

.align 8
gdt:
    .quad 0
    .quad 0x00CF9A000000FFFF # 0x08: 32-bit code, base 0, limit 4 GiB
    .quad 0x00CF92000000FFFF # 0x10: data, base 0, limit 4 GiB
gdt_end:
gdt_desc:
    .word gdt_end - gdt - 1
    .long gdt

dap:
    .byte 0x10, 0
dap_count: .word 0
dap_off:   .word 0
    .word BOUNCE_SEG
dap_lba:   .long 0, 0

boot_drive: .byte 0
sect_shift: .byte 9
retries:    .byte 0
want_count: .word 0
err_ah:     .byte 0
.align 4
entry:      .long 0
kernel_base: .long 0 # first sector of the kernel image
unit_mask:  .long 511 # sector size - 1
bounce_units: .long 0 # sectors per bounce-buffer read
hdr_units:  .long 0 # sectors holding the first 4 KiB
phnum:      .long 0
seg_off:    .long 0
seg_dest:   .long 0
seg_filesz: .long 0
seg_memsz:  .long 0
phbuf:      .skip MAX_PHDRS * 32

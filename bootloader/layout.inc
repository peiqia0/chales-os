# Disk and memory layout shared by stage 1 and stage 2 (and image.sh).
.equ STAGE2_LBA,      1
.equ STAGE2_SECTORS,  15
.equ KERNEL_LBA,      16

.equ STAGE2_ADDR,     0x7E00     # stage 2
.equ BOUNCE_SEG,      0x1000
.equ BOUNCE_ADDR,     0x10000
.equ BOUNCE_SECTORS,  64

.equ BOUNCE_CD_SECTORS, (BOUNCE_SECTORS * 512) >> 11

.equ BOOT_INFO_FILE_LBA,  0x7C0C
.equ KERNEL_CD_OFFSET,    (KERNEL_LBA * 512) >> 11   # kernel's offset in CD sectors

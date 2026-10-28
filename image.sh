#!/bin/sh
# Builds chales-os.img: a raw BIOS boot disk with our own bootloader.
set -e
. ./build.sh

$MAKE -C bootloader HOST="$HOST"

KERNEL=sysroot/boot/chales-os.kernel
IMG=chales-os.img
KERNEL_LBA=$(sed -n 's/^\.equ KERNEL_LBA, *\([0-9]*\).*/\1/p' bootloader/layout.inc)

HEADS=64
SPT=32
CYL_SECTORS=$((HEADS * SPT))

KERNEL_SIZE=$(wc -c < "$KERNEL")
USED=$((KERNEL_LBA + (KERNEL_SIZE + 511) / 512))
CYLS=$(((USED + CYL_SECTORS - 1) / CYL_SECTORS))
SECTORS=$((CYLS * CYL_SECTORS))

dd if=/dev/zero of="$IMG" bs=512 count="$SECTORS" 2>/dev/null
dd if=bootloader/stage1.bin of="$IMG" bs=512 seek=0 conv=notrunc 2>/dev/null
dd if=bootloader/stage2.bin of="$IMG" bs=512 seek=1 conv=notrunc 2>/dev/null
dd if="$KERNEL" of="$IMG" bs=512 seek="$KERNEL_LBA" conv=notrunc 2>/dev/null

# Partition entry 1 (offset 446): active, type 0x83, LBA 1 to the end of the disk.
le32() { printf "\\$(printf %03o $(($1 & 255)))\\$(printf %03o $((($1 >> 8) & 255)))\\$(printf %03o $((($1 >> 16) & 255)))\\$(printf %03o $((($1 >> 24) & 255)))"; }
chs() { # lba -> 3 CHS bytes
  c=$(($1 / CYL_SECTORS)); h=$((($1 % CYL_SECTORS) / SPT)); s=$((($1 % SPT) + 1))
  printf "\\$(printf %03o $h)\\$(printf %03o $((s | ((c >> 2) & 0xC0))))\\$(printf %03o $((c & 255)))"
}
{
  printf '\200'; chs 1; printf '\203'; chs $((SECTORS - 1)); le32 1; le32 $((SECTORS - 1))
} | dd of="$IMG" bs=1 seek=446 conv=notrunc 2>/dev/null

echo "built $IMG ($SECTORS sectors)"

#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/chales-os.kernel isodir/boot/chales-os.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "chales-os" {
	multiboot /boot/chales-os.kernel
}
EOF
grub-mkrescue -o chales-os.iso isodir
cp chales-os.iso builds/0.5.0.iso
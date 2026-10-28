#!/bin/sh
set -e
. ./image.sh

rm -rf isodir
mkdir -p isodir
cp chales-os.img isodir/chales-os.img

xorriso -as mkisofs -quiet -V CHALES_OS \
  -b chales-os.img -no-emul-boot -boot-load-size 16 -boot-info-table \
  -o chales-os.iso isodir
echo "built chales-os.iso"

#!/bin/sh
set -e
. ./config.sh

for PROJECT in $PROJECTS; do
  (cd $PROJECT && $MAKE clean)
done
cd bootloader && $MAKE clean
rm -rf sysroot
rm -rf isodir
rm -rf chales-os.iso
rm -rf chales-os.img

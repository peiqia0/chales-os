#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom chales-os.iso -audiodev pa,id=snd -machine pcspk-audiodev=snd

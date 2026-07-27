#!/bin/sh
set -e
. ./iso.sh

# Create a temporary Bochs configuration file to boot the generated ISO.
cat > bochsrc.bxrc << 'EOF'
megs: 32
romimage: file=/usr/share/bochs/BIOS-bochs-latest
vgaromimage: file=/usr/share/bochs/VGABIOS-lgpl-latest.bin
ata0-master: type=cdrom, path="chales-os.iso", status=inserted
boot: cdrom
clock: sync=realtime
display_library: x
log: bochs.log
panic: action=ask
debug: action=ignore
EOF

bochs -q -f bochsrc.bxrc

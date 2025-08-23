# chales-os
a hobby operating system/kernel
## Requirements
- make
- open watcom compiler
- nasm
- qemu
## How to run
compile the file with 
```
make -s
```
run the floppy image with
```
qemu-system-i386 -fda build/main_floppy.img
```

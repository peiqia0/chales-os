# chales-os
a hobby operating system/kernel
## Requirements
- make
- open watcom compiler
- nasm
- qemu
- Any linux/debian based system
## How to run
although the code isn't complete yet and only can print "hello, world" you can still try the code out for yourself
compile the file with 
```
make -s
```
run the floppy image with
```
qemu-system-i386 -fda build/main_floppy.img
```

# Chales-OS
This is a a hobby os based on the osdev wiki for the i386 architecture. This probably sucks since half of it is me messing around with stuff and the other half is me learning directly off the osdev wiki.
 ## Features
- i386 kernel — GDT, IDT, interrupt handling, and hardware abstraction
- VGA text terminal — 80×25 scrolling display with color support
- PS/2 keyboard driver — interrupt-driven keyboard input with buffer
- PIT timer — programmable interval timer via IRQ0
- libk — small freestanding library for the kernel (string, printk, kmalloc)
- Newlib — user programs use newlib on top of the kernel's syscalls
- RAMFS — in-memory filesystem supporting files and directories
- Interactive shell — command-line interface with built-in commands
- ELF executable loader — load and runs elf binaries
- Scheduler - Simple round robin scheduler to make multitasking possible
## Shell Commands

| Command | Description |
|---|---|
| `help` | List available commands |
| `clear` | Clear the screen |
| `echo <text>` | Print text to the terminal |
| `ls [path]` | List files in a directory |
| `cat <file>` | Display file contents |
| `touch <file>` | Create an empty file |
| `write <file> <text>` | Write text to a file |
| `mkdir <dir>` | Create a directory |
| `rmdir <dir>` | Remove a directory |
| `rm <file>` | Delete a file |
| `uname` | Display system information |
| `reboot` | Reboot the system |

## Building

### Prerequisites

You need an **i686 cross-compiler** (i686-elf-gcc) and the standard build tools. See the [OSDev Wiki cross-compiler guide](https://wiki.osdev.org/GCC_Cross-Compiler) for setup instructions.

You also need a **newlib** built for `i686-elf` (user programs link against it) and `grub-mkrescue`, `xorriso`, and `mtools` to build the ISO. Point the build at newlib with `NEWLIB_PREFIX` (default `$HOME/opt/newlib`, expects `$NEWLIB_PREFIX/i686-elf/lib/libc.a`).

```sh
# Install system headers
./headers.sh

# Build libc and kernel
./build.sh

# Create a bootable ISO
./iso.sh
```

### Running in QEMU

```sh
./qemu.sh
```

This builds the ISO and launches it with `qemu-system-i386`.

Alternatively, boot the pre-built ISO directly:

```sh
qemu-system-i386 -cdrom builds/0.4.1.iso
```
## Newlib

The kernel implements the syscalls newlib needs, and `libc/newlib/newlib_syscalls.c` is the user-space glue (`libchalessys.a`). It exports both the POSIX names (`write`, `sbrk`, ... as used by a stock bare-metal `i686-elf` newlib) and the underscored ones (`_write`, `_sbrk`, ...).

| Syscall | Number | Notes |
|---|---|---|
| open2 / lseek / fstat / stat | 22 / 23 / 24 / 25 | newlib `O_*` flag values; return `-errno` |
| isatty / getpid / kill | 26 / 27 / 28 | any signal terminates the target task |
| times / gettimeofday | 29 / 30 | time since boot (no RTC yet) |

File descriptors 0/1/2 are the console, RAMFS files start at 3.

`execv`/`execve` (syscall 21) is exec-and-wait: every ELF is linked at 0x300000 and shares one user stack, so the kernel blocks the caller, saves the parts of its image, stack and heap break that the child would overwrite, runs the child to completion, then restores them. Unlike POSIX it returns 0 on success (-1 if the program could not be started); `wait` (syscall 31) then returns the child's pid and a wait()-style status. `fork` is not implemented (`EAGAIN`).

Every program in `userland/` is built against newlib, and `/bin/nltest` exercises the syscall layer (files, heap, time, stdin, exec-and-wait). `libc/` now only holds `libk` (the kernel's library) and the newlib glue. Build with:

```sh
NEWLIB_PREFIX=$HOME/opt/newlib ./build.sh
```

Then run `nltest` from the shell.

## Plans
- graphics
- real file system
- usb

## License

This project is under the GNU General Public liscence

## Acknowledgements

- **[sortie](https://github.com/sortie)** — code and the OSDev Meaty Skeleton tutorial
- **[BlaiseTara](https://github.com/BlaiseTara/)** — emotional support

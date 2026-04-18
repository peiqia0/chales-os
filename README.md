# Chales-OS
This is a a hobby os based on the osdev wiki for the i386 architecture. This probably sucks since half of it is me messing around with stuff and the other half is me learning directly off the osdev wiki.
 ## Features
- i386 kernel — GDT, IDT, interrupt handling, and hardware abstraction
- VGA text terminal — 80×25 scrolling display with color support
- PS/2 keyboard driver — interrupt-driven keyboard input with buffer
- PIT timer — programmable interval timer via IRQ0
- Custom libc — minimal C standard library (string, stdio, stdlib)
- Heap allocator — malloc/free with a simple first-fit allocator
- RAMFS — in-memory filesystem supporting files and directories
- Interactive shell — command-line interface with built-in commands
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

You also need `grub-mkrescue`, `xorriso`, and `mtools` to build the ISO.

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

## License

This project is under the GNU General Public liscence

## Acknowledgements

- **[sortie](https://github.com/sortie)** — code and the OSDev Meaty Skeleton tutorial
- **[BlaiseTara](https://github.com/BlaiseTara/)** — emotional support

/* The bootloader will look at this image and start execution at the symbol
   designated at the entry point. */
ENTRY(_start)
SECTIONS
{
	. = 1M;
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(EXCLUDE_FILE(*user/main.o* *printf.o* *stdlib/malloc.o* *string/memcpy.o* *string/memset.o* *string/memmove.o* *string/memcmp.o*) .text)
	}
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(EXCLUDE_FILE(*user/main.o* *printf.o* *stdlib/malloc.o* *string/memcpy.o* *string/memset.o* *string/memmove.o* *string/memcmp.o*) .rodata)
	}
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(EXCLUDE_FILE(*user/main.o* *printf.o* *stdlib/malloc.o* *string/memcpy.o* *string/memset.o* *string/memmove.o* *string/memcmp.o*) .data)
	}

	.user BLOCK(4K) : ALIGN(4K)
	{
		_user_start = .;
		user/main.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.*)
		../libc/stdio/printf.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.*)
		../libc/stdlib/malloc.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.*)
		../libc/string/memcpy.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.*)
		../libc/string/memset.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.*)
		../libc/string/memmove.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.*)
		../libc/string/memcmp.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.*)
		*(.user)
		*(.user.*)
		_user_end = .;
	}
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(EXCLUDE_FILE(*user/main.o* *printf.o* *stdlib/malloc.o* *string/memcpy.o* *string/memset.o* *string/memmove.o* *string/memcmp.o*) COMMON)
		*(EXCLUDE_FILE(*user/main.o* *printf.o* *stdlib/malloc.o* *string/memcpy.o* *string/memset.o* *string/memmove.o* *string/memcmp.o*) .bss)
		_heap_start = .;
		. += 64K;  /* 64K kernel heap (kmalloc), supervisor-only */
		_heap_end = .;
		. = ALIGN(4K);
		_user_heap_start = .;
		. += 128K;  /* 128K user heap (malloc/sbrk), marked PTE_USER */
		_user_heap_end = .;
		*(.bootstrap_stack)
		*(.kernel_stack)
	}
}
ENTRY(_start)
SECTIONS
{
	. = 1M;
	_kernel_start = .;
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(EXCLUDE_FILE(*user/*.o*) .text)	}
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(EXCLUDE_FILE(*user/*.o*) .rodata)
	}
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(EXCLUDE_FILE(*user/*.o*) .data)
	}

	.user BLOCK(4K) : ALIGN(4K)
	{
		_user_start = .;
		user/*.o(.text .text.* .rodata .rodata.* .data .data.* .bss .bss.*)
		*(.user)
		*(.user.*)
		_user_end = .;
	}
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(EXCLUDE_FILE(*user/*.o*) COMMON)
		*(EXCLUDE_FILE(*user/*.o*) .bss)
		_heap_start = .;
		. += 64K;  /* 64K kernel heap (kmalloc), supervisor-only */
		_heap_end = .;
		. = ALIGN(4K);
		_user_heap_start = .;
		. += 128K;  /* 128K user heap (malloc/sbrk), marked PTE_USER */
		_user_heap_end = .;
		*(.bootstrap_stack)
		*(.kernel_stack)
		_kernel_end = .;
	}
}

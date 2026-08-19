ENTRY(_start)
SECTIONS
{
	/* Load base for dynamically-loaded user ELF binaries: 3MiB. This is
	 * comfortably above the kernel image (linked at 1MiB, currently well
	 * under 1MiB in size — see _kernel_start/_kernel_end in
	 * kernel/arch/i386/linker.ld) and comfortably below the 4MiB
	 * identity-map / ELF load limit enforced by
	 * kernel/arch/i386/elf.c's ELF_LOAD_LIMIT. If the kernel image ever
	 * grows past ~3MiB, or you add more user binaries that also target
	 * this address, this will need to move or become configurable.
	 *
	 * Deliberately NOT page-aligning each section here: the kernel's ELF
	 * loader copies segment bytes with memcpy() rather than mmap(), so
	 * on-disk page alignment buys nothing — it only bloats the file
	 * (each 4K-aligned section boundary pads the file out to the next
	 * page). ramfs caps files at RAMFS_MAX_FILESIZE (8KiB), so keeping
	 * this dense matters. */
	. = 0x300000;

	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(COMMON) *(.bss) }
}

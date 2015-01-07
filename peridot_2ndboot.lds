OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Vectors)

SECTIONS
{
	. = 0xFFFF0200;
	. = ALIGN(4);
	.text :
	{
		obj/startup.o	(.text)
		*(.text)
	}

	. = ALIGN(4);
	.rodata : { *(.rodata*) }

	. = ALIGN(4);
	.data : { *(.data*) }

	. = ALIGN(4);
	.got : { *(.got) }

	_end = .;

	. = ALIGN(4);
	.bss :
	{
		__bss_start = .;
		*(.bss*)
		. = ALIGN(4);
		__bss_end__ = .;
	}
}


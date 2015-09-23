OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Vectors)

SECTIONS
{
	. = 0xFFFF0200;
	. = ALIGN(4);
	.text :
	{
		obj/startup_aarch32.o	(.text)
		*(.text)
	}

	. = ALIGN(4);
	.rodata : { *(.rodata*) }

	. = ALIGN(4);
	.data : { *(.data*) }

	. = ALIGN(4);
	.got : { *(.got) }

	_end = .;

	__bss_start__ = .;
	.bss_start (OVERLAY) : {
		KEEP(*(.__bss_start));
		__bss_base = .;
	}
	.bss __bss_base (OVERLAY) : {
		*(.bss*)
		. = ALIGN(4);
		__bss_limit = .;
	}
	.bss_end __bss_limit (OVERLAY) : {
		KEEP(*(.__bss_end));
	}
	__bss_end__ = .;
}


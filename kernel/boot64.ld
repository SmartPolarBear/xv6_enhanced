OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(_start)

SECTIONS
{
    . = 0x00100000;

	.text : AT(0x00100000) {
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}

	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	. = ALIGN(0x1000);

	.data : {
		*(.data)
	}

	. = ALIGN(0x1000);

	.bss : {
		*(.bss)
	}

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack .debug_* .comment)
	}
}

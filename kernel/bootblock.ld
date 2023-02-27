OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(start)

SECTIONS
{
	. = 0x7c00;

	.text : AT(0x7c00) {
		*(.text)
	}

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack .debug_* .comment)
	}
}

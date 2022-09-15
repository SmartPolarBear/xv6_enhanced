OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_ulib_start)

SECTIONS {
    /* Load programs at this address: "." means the current address */
    . = 0x0000;

    .text : AT(0x0000) {
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }

    /* Adjust the address for the data segment to the next page */
    . = ALIGN(0x1000);

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack .comment)
    }
}
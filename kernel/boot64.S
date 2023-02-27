#include "asm.h"
#include "memlayout.h"
#include "mmu.h"

.code32
.globl _start
_start:
    cli

    # clean up screen
    mov $4000,%ecx
    mov $0xb8000,%edi
    mov $0x07200720,%eax
    rep stosw

    # print starting xv6
    movl $0x07540753,0xb8000
    movl $0x07410752,0xb8004
    movl $0x07490754,0xb8008
    movl $0x0747074e,0xb800c
    movl $0x07580720,0xb8010
    movl $0x07360756,0xb8014


    call boot64main

    # output 'ERROR' for error detection
    movl $0x1f521f45,0xb8000
    movl $0x1f4f1f52,0xb8004
    movl $0x1f211f52,0xb8008
spin:
    jmp spin
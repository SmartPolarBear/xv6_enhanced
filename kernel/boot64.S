#include "asm.h"
#include "memlayout.h"
#include "mmu.h"

.code32
.globl _start
_start:
    movl $0x07690748,0xb8016
    call boot64main
spin:
    jmp spin
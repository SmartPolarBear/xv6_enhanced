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

    # zero 4 pages for our bootstrap page tables
    xor %eax, %eax
    mov $0x1000, %edi
    mov $0x5000, %ecx
    rep stosb

    # P4ML[0] -> 0x2000 (PDPT-A)
    mov $(0x2000 | 3), %eax
    mov %eax, 0x1000

    # P4ML[511] -> 0x3000 (PDPT-B)
    mov $(0x3000 | 3), %eax
    mov %eax, 0x1FF8

    # PDPT-A[0] -> 0x4000 (PD)
    mov $(0x4000 | 3), %eax
    mov %eax, 0x2000

    # PDPT-B[510] -> 0x4000 (PD)
    mov $(0x4000 | 3), %eax
    mov %eax, 0x3FF0

    # PD[0..511] -> 0..1022MB
    mov $0x83, %eax
    mov $0x4000, %ebx

    mov $512, %ecx
ptbl_loop:
    mov %eax, (%ebx)
    add $0x200000, %eax
    add $0x8, %ebx
    dec %ecx
    jnz ptbl_loop

    mov $0x1000,%eax
    mov %eax,%cr3

    lgdt (gdtr64 - _start + 0x00100000)

    # Enable PAE - CR4.PAE=1
    mov %cr4, %eax
    bts $5, %eax
    mov %eax, %cr4

    # enable long mode - EFER.LME=1
    mov $0xc0000080, %ecx
    rdmsr
    bts $8, %eax
    wrmsr

    mov %cr0, %eax
    bts $31, %eax
    mov %eax, %cr0

    ljmp $8,$(entry64_low - _start + 0x00100000)

.align 16
gdtr64:
    .word gdt64_end - gdt64_begin - 1;
    .quad gdt64_begin - _start + 0x00100000

.align 16
gdt64_begin:
    .long 0x00000000 # 0: null desc
    .long 0x00000000
    .long 0x00000000 # 1: Code, R/X, Nonconforming
    .long 0x00209800
    .long 0x00000000 # 2: Data, R/W, Expand Down
    .long 0x00009000
gdt64_end:

.align 16
.code64
entry64_low:
    movq $entry64_high, %rax
    jmp *%rax

.section .text
entry64_high:
    # print starting xv6
    movl $0x07540753,0xb8000
    movl $0x07410752,0xb8004
    movl $0x07490754,0xb8008
    movl $0x0747074e,0xb800c
    movl $0x07580720,0xb8010
    movl $0x07360756,0xb8014

    xor %rax, %rax
    mov %ax, %ss
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $0xFFFFFFFF80010000, %rax
    mov %rax, %rsp

    call boot64main

    # output 'ERROR' for error detection
    movl $0x1f521f45,0xb8000
    movl $0x1f4f1f52,0xb8004
    movl $0x1f211f52,0xb8008
spin:
    jmp spin

.global wrmsr
wrmsr:
    mov %rdi, %rcx     // arg0 -> msrnum
    mov %rsi, %rax     // val.low -> eax
    shr $32, %rsi
    mov %rsi, %rdx     // val.high -> edx
    wrmsr
    retq

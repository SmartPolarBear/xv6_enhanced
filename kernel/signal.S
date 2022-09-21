#include "syscall.h"
#include "traps.h"

.global trampoline_head
.global trampoline_tail

trampoline_head:
movl $SYS_sigreturn, %eax
int $T_SYSCALL
ret
trampoline_tail:
nop
nop
nop
#include "syscall.h"
#include "traps.h"

.global trampoline_start
.global trampoline_end

trampoline_start:
movl $SYS_sigreturn, %eax
int $T_SYSCALL
ret
trampoline_end:
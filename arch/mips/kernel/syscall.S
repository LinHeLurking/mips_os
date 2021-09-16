#include "asm.h"

LEAF(invoke_syscall)
    // TODO syscall entry
    .set noreorder
    move v0, a0
    move a0, a1
    move a1, a2
    move a2, a3
    addiu sp, sp, -8
    sw ra, 0(sp)
    syscall
    nop
    lw ra, 0(sp)
    addiu sp, sp, 8
    jr ra
    nop
    .set reorder
END(invoke_syscall)
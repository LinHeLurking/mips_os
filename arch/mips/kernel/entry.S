#include "asm.h"



.equ    KERNEL, 0
.equ    USER,   156
.equ    PT, 312
.equ    PID, 4408

.data 
.global current_running
current_running: .word 0x0

.global slice_size
slice_size: .word 150000

.global real_exception_entry
real_exception_entry: .word 0x80000180

.text

.macro STI
    nop
.endm

.macro CLI
    nop
.endm

LEAF(asm_start)
    mtc0    zero, CP0_STATUS
    mtc0    zero, CP0_COUNT
    mtc0    zero, CP0_WATCHLO
    mtc0    zero, CP0_WATCHHI

    mfc0    t0, CP0_CONFIG
    and     t0, ~0x7
    ori     t0, 0x2
    mtc0    t0, CP0_CONFIG
    
    jr  ra
END(asm_start)

.macro SAVE_CONTEXT offset
.set noat
    // TODO save context
    // prepare a part of stack space for swap regs
    // $k0 does not need to be saved
    lw $26,current_running
    addiu $26,$26, \offset

    // saving general registers
    sw $1, 4($26)
    sw $2, 8($26)
    sw $3, 12($26)
    sw $4, 16($26)
    sw $5, 20($26)
    sw $6, 24($26)
    sw $7, 28($26)
    sw $8, 32($26)
    sw $9, 36($26)
    sw $10, 40($26)
    sw $11, 44($26)
    sw $12, 48($26)
    sw $13, 52($26)
    sw $14, 56($26)
    sw $15, 60($26)
    sw $16, 64($26)
    sw $17, 68($26)
    sw $18, 72($26)
    sw $19, 76($26)
    sw $20, 80($26)
    sw $21, 84($26)
    sw $22, 88($26)
    sw $23, 92($26)
    sw $24, 96($26)
    sw $25, 100($26)
    // sw $26, 104($26)
    // sw $27, 108($26)
    sw $28, 112($26)
    sw $29, 116($26)
    sw $30, 120($26)
    sw $31, 124($26) 

    // saving cp0 registers 
    mfc0 $27, CP0_STATUS
    sw $27, 128($26)
    mfhi $27
    sw $27, 132($26)
    mflo $27
    sw $27,136($26)
    mfc0 $27, CP0_BADVADDR
    sw $27, 140($26)
    mfc0 $27,CP0_CAUSE
    sw $27, 144($26)
    mfc0 $27, CP0_EPC
    sw $27, 148($26)


.endm

.macro RESTORE_CONTEXT offset
.set noat
    // TODO restore context

    // prepare for swapping
    lw $26,current_running
    addiu $26, $26, \offset

    // restore general registers
    lw $1, 4($26)
    lw $2, 8($26)
    lw $3, 12($26)
    lw $4, 16($26)
    lw $5, 20($26)
    lw $6, 24($26)
    lw $7, 28($26)
    lw $8, 32($26)
    lw $9, 36($26)
    lw $10, 40($26)
    lw $11, 44($26)
    lw $12, 48($26)
    lw $13, 52($26)
    lw $14, 56($26)
    lw $15, 60($26)
    lw $16, 64($26)
    lw $17, 68($26)
    lw $18, 72($26)
    lw $19, 76($26)
    lw $20, 80($26)
    lw $21, 84($26)
    lw $22, 88($26)
    lw $23, 92($26)
    lw $24, 96($26)
    lw $25, 100($26)
    // lw $26, 104($26)
    // lw $27, 108($26)
    lw $28, 112($26)
    lw $29, 116($26)
    lw $30, 120($26)
    lw $31, 124($26)

    // restoring cp0 registers
    lw $27, 128($26)
    // mtc0 $27, CP0_STATUS
    lw $27, 132($26)
    mthi $27
    lw $27,136($26)
    mtlo $27
    lw $27, 140($26)
    mtc0 $27, CP0_BADVADDR
    lw $27, 144($26)
    // mtc0 $27,CP0_CAUSE
    lw $27, 148($26)
    mtc0 $27, CP0_EPC
.endm

NESTED(do_scheduler, 0, ra)
    SAVE_CONTEXT(KERNEL)
    jal     scheduler
    nop  
    RESTORE_CONTEXT(KERNEL)
    jr      ra
    nop
END(do_scheduler)

.global exception_handler_begin
.global exception_handler_end

NESTED(exception_handler_entry, 0, sp)   
exception_handler_begin:
    mfc0 k1, CP0_EPC


    // TODO close interrupt
    // jmp exception_handler[i] which decided by CP0_CAUSE
    // Leve2 exception Handler.

    SAVE_CONTEXT(USER)

    // jmp to exception handler
    mfc0 k0, CP0_CAUSE
    srl k0, 2
    andi k0, k0, 31
    // int 
    li k1, 0
    nop
    beq k0, k1, handle_int
    nop 
    // TLB 
    li k1, 2
    beq k0, k1, TLB_exception_handler_entry
    li k1, 3
    beq k0, k1, TLB_exception_handler_entry
    // syscall 
    li k1, 8
    nop
    beq k0, k1, handle_syscall
    nop
    // others 
    j handle_other
    nop
    
    j exception_handler_exit

exception_handler_end:
END(exception_handler_entry)

LEAF(exception_handler_exit)
    // restore the context 
    RESTORE_CONTEXT(USER)

    li k0, 0x8803
    nop
    mtc0 k0, CP0_STATUS
    eret
END(exception_handler_exit)

NESTED(handle_int, 0, sp)
    // interrupt handler
    // Leve3 exception Handler.
    RESTORE_CONTEXT(KERNEL)
    addiu sp, sp, -8
    mfc0 a0, CP0_STATUS
    mfc0 a1, CP0_CAUSE
    jal interrupt_helper
    nop
    addiu sp, sp, 8
    SAVE_CONTEXT(KERNEL)
    j exception_handler_exit
END(handle_int)

NESTED(handle_syscall, 0, sp)
    // system call handler
    // EPC <- EPC + 4
    lw k0, current_running
    addiu k0, k0, USER
    lw k1, 148(k0)
    addiu k1, k1, 4
    sw k1, 148(k0)
    // set $sp as current_running->kernel_context.regs[29]
    lw k0, current_running
    lw sp, 120(k0) 
    move fp, sp
    // move parameters
    move a3, a2 
    move a2, a1 
    move a1, a0 
    move a0, v0
    addiu sp, sp, -16
    jal system_call_helper
    nop
    addiu sp, sp, 16
    j exception_handler_exit
END(handle_syscall)

NESTED(handle_other, 0, sp)
    // other exception handler
    mfc0 k1, CP0_EPC
    j exception_handler_exit
END(handle_other)

// only using $k0 in this leaf procedure
LEAF(set_count_compare)
    lw k0, slice_size
    mtc0 k0, CP0_COMPARE
    addiu k0, $0, 0
    mtc0 k0, CP0_COUNT

    jr ra
    nop
END(set_count_compare)

LEAF(enable_int)
    add k1, ra, $0
    jal set_count_compare
    nop 
    add ra, k1, $0
    mtc0 ra, CP0_EPC
    li k0, 0x8003
    nop
    mtc0 k0, CP0_STATUS
    eret
END(enable_int)

LEAF(jmp_exception_handler_entry)
    la k0, exception_handler_entry
    jr k0
    nop
END(jmp_exception_handler_entry)

LEAF(jmp_TLB_exception_handler_entry)
    j TLB_exception_handler_entry
    nop
END(jmp_TLB_exception_handler_entry)

LEAF(TLB_exception_handler_entry)
    // No need for saving context if it is TLB refill
    // Try to find a page in page table and write it into TLB
    // If not found, then save context and jmp to page-missing function
    mfc0 k0, CP0_BADVADDR
    move k1, k0 
    srl k0, 21 // first level index
    sll k0, 2
    srl k1, 13
    andi k1, k1, 510 // second level index of that even page
    sll k1, 3  // sizeof(PTE_t)==8, so addr_offset==index*8. now k1 is the offset of second level page
    lw t0, current_running
    lw t2, PID(t0) // pid at t2
    add k0, k0, t0 
    lw k0, PT(k0)  // now k0 is the first level PTE pointer, pointing at some second page
    beqz k0, 2f
    add k0, k0, k1 
    lw t0, 0(k0)   // now t0 is the vpn
    beqz t0, 1f 
    lw t0, 4(k0)   // entrylo0 at t0
    lw t1, 12(k0)  // entrylo1 at t1
    mfc0 k0, CP0_BADVADDR
    li   t3, 0xffffe000
    and  k0, k0, t3
    or   k0, k0, t2 // entryhi at k0
    mtc0 t0, CP0_ENTRYLO0 
    mtc0 t1, CP0_ENTRYLO1
    mtc0 k0, CP0_ENTRYHI
    mtc0 $0, CP0_PAGEMASK
    // tlbp to check whether it is tlb refill or tlb invalid
    tlbp 
    mfc0 k1, CP0_INDEX
    srl  k1, 31 
    beq  k1, $0, 1f // found 
    // not found 
    tlbwr
    // used for debugging
    tlbp 
    mfc0 k1, CP0_INDEX
    tlbr 
    mfc0 t0, CP0_ENTRYHI
    mfc0 t1, CP0_ENTRYLO0
    mfc0 t2, CP0_ENTRYLO1
    j 3f
1:  // found
    tlbwi
    j 3f
2:  // page missing
    SAVE_CONTEXT(USER)
    lw k0, current_running
    lw sp, 120(k0) // set $sp as current_running->kernel_context.regs[29]
    move fp, sp
    mfc0 a0, CP0_BADVADDR
    srl  a0, 13  // vpn2
    addiu sp, sp, -4
    jal page_miss_handler
    nop
    addiu sp, sp, 4
    RESTORE_CONTEXT(USER)
3:  // exit 
    eret
    nop
END(TLB_exception_handler_entry)

LEAF(do_reboot)
    li      ra, 0xa0800000
    mtc0    $0, CP0_STATUS
    jr      ra
END(do_reboot)


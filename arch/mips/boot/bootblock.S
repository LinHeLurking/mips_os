.text
.global main

main:
	# use your own code in here!
	# 1) call BIOS print string "It's bootblock!
	# 2) call BIOS read kernel in SD card.
	# 3) jump to kernel.

    # 1) call BIOS print string "It's bootblock!"

    # print msg using BIOS
    la $a0,msg
    lw $s0,printstr 
    jal $s0

    # 2) call BIOS read kernel in SD card and jump to kernel start
   
    
    # read os_size which is located in the final 4 bytes of the first sector
    addiu $sp,$sp,-4
    addiu $a0,$sp,0
    li $a1,508
    li $a2,4
    lw $s0,read_sd_card 
    jal $s0 
 
    # moving kernel code into memory
    lw $a0,kernel 
    li $a1,0x200 
    lw $a2,($sp)
    addiu $sp,$sp,4
    lw $ra,kernel 
    jal $s0 

    # jump to subroutine of kernel
    lw $s2, kernel_main
    li $sp, 0xa0f00000
    jal $s2 

# 4) while(1) --> stop here
stop:
	j stop


.data

msg: .ascii "It's a bootloader...\n"

# 1. PMON read SD card function address
# read_sd_card();
read_sd_card: .word 0x80011000

# 2. PMON print string function address
# printstr(char *string)
printstr: .word 0x80011100

# 3. PMON print char function address
# printch(char ch)
printch: .word 0x80011140

# 4. kernel address (move kernel to here ~)
kernel : .word 0xa0800200

# 5. kernel main address (jmp here to start kernel main!)
kernel_main : .word 0xa0800200

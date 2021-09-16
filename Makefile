CC = mipsel-linux-gcc

all: clean createimage image asm # floppy
dbgall: clean dbgcreateimage dbgmain image asm # floppy

SRC_BOOT 	= ./arch/mips/boot/bootblock.S

SRC_ARCH	= ./arch/mips/kernel/entry.S ./arch/mips/kernel/syscall.S ./arch/mips/pmon/common.c 
SRC_DRIVER	= ./drivers/screen.c ./drivers/mac.c
SRC_INIT 	= ./init/main.c
SRC_INT		= ./kernel/irq/irq.c
SRC_LOCK	= ./kernel/locking/lock.c ./kernel/locking/barrier.c ./kernel/locking/cond.c ./kernel/locking/sem.c
SRC_MM          = ./kernel/mm/memory.c
SRC_SCHED	= ./kernel/sched/sched.c ./kernel/sched/queue.c ./kernel/sched/time.c
SRC_SYSCALL	= ./kernel/syscall/syscall.c
SRC_LIBS	= ./libs/string.c ./libs/printk.c ./libs/algo.c ./libs/mailbox.c ./libs/input.c
SRC_SHELL   = ./kernel/shell/*
SRC_FS      = ./kernel/fs/*

SRC_TEST	= ./test/test.c ./test/test_net/test_mac3.c ./test/test_fs/test_fs.c
SRC_IMAGE	= ./tools/createimage.c

bootblock: $(SRC_BOOT)
	${CC} -G 0 -O2 -fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800000 -N -o bootblock $(SRC_BOOT) -nostdlib -e main -Wl,-m -Wl,elf32ltsmip -T ld.script

main : $(SRC_ARCH) $(SRC_DRIVER) $(SRC_INIT) $(SRC_INT) $(SRC_LOCK) $(SRC_MM) $(SRC_SCHED) $(SRC_SYSCALL) $(SRC_LIBS) $(SRC_SHELL) $(SRC_FS) $(SRC_TEST)
	${CC} -G 0 -O0 -std=c99 -Iinclude -Ilibs -Iarch/mips/include -Idrivers -Iinclude/os -Iinclude/sys -Iinclude/shell -Iinclude/libs -Itest -Itest/test_fs/ \
	-fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800200 -N -o main \
	$(SRC_ARCH) $(SRC_DRIVER) $(SRC_INIT) $(SRC_INT) $(SRC_LOCK) $(SRC_MM) $(SRC_SCHED) $(SRC_SYSCALL) $(SRC_PROC) $(SRC_LIBS) $(SRC_SHELL) $(SRC_FS) $(SRC_TEST) -L libs/ -lepmon -nostdlib -Wl,-m -Wl,elf32ltsmip -T ld.script		

createimage: $(SRC_IMAGE)
	gcc $(SRC_IMAGE) -o createimage

dbgbootblock: $(SRC_BOOT)
	${CC} -g -G 0 -O2 -DDEBUG -fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800000 -N -o bootblock $(SRC_BOOT) -nostdlib -e main -Wl,-m -Wl,elf32ltsmip -T ld.script

dbgmain : $(SRC_ARCH) $(SRC_DRIVER) $(SRC_INIT) $(SRC_INT) $(SRC_LOCK) $(SRC_MM) $(SRC_SCHED) $(SRC_SYSCALL) $(SRC_LIBS) $(SRC_SHELL) $(SRC_FS) $(SRC_TEST)
	${CC} -g -G 0 -O0 -DDEBUG -std=c99 -Iinclude -Ilibs -Iarch/mips/include -Idrivers -Iinclude/os -Iinclude/sys -Iinclude/shell -Iinclude/libs -Itest -Itest/test_fs/ \
	-fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800200 -N -o main \
	$(SRC_ARCH) $(SRC_DRIVER) $(SRC_INIT) $(SRC_INT) $(SRC_LOCK) $(SRC_MM) $(SRC_SCHED) $(SRC_SYSCALL) $(SRC_PROC) $(SRC_LIBS) $(SRC_SHELL) $(SRC_FS) $(SRC_TEST) -L libs/ -lepmon -nostdlib -Wl,-m -Wl,elf32ltsmip -T ld.script		

dbgcreateimage: $(SRC_IMAGE)
	gcc -g $(SRC_IMAGE) -DDEBUG -o createimage

image: createimage bootblock main
	./createimage --extended bootblock main

clean:
	rm -rf bootblock image createimage main *.o kernel.txt

floppy:
	sudo fdisk -l /dev/sdc
	sudo dd if=image of=/dev/sdc conv=notrunc

asm:
	mipsel-linux-objdump -d main > kernel.txt

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit 
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include "irq.h"
#include "mm.h"
// #include "test.h"
#include "stdio.h"
#include "sched.h"
#include "screen.h"
#include "common.h"
#include "syscall.h"
#include "shell.h"
#include "input.h"
#include "mac.h"
#include "fs.h"

/* current running task PCB */
extern pcb_t *current_running;
// pid_t process_id;

// clear and reset the count and compare regs,
// interrupts are not enabled here.
extern void set_count_compare();

// enable interrupts using this fucntion
extern void enable_int();

// do not use __asm__ to jmp for the mips calling convension of $sp operations
extern void jmp_exception_handler_entry();
extern void jmp_TLB_exception_handler_entry();

/* kernel_context.regs[31] for all user process */
extern void exception_handler_exit();

static void init_pready_queue()
{
	for (int i = 0; i < __MAX_PRIORITY_NUM; ++i)
	{
		priority_ready_queue_counter[i] = 0;
		priority_ready_queue_counter_uppper_bound[i] = 0;
	}
}

static void init_pcb()
{

	uint32_t kernel_stack_top = STK_UNMAPPED_BASE;
	uint32_t user_stack_top = STK_UNMAPPED_BASE - NUM_MAX_TASK * (STK_MAX_KER_PROC_OCC);
	for (int i = 0; i < NUM_MAX_TASK; ++i)
	{
		reset_pcb(&pcb[i]);
		pcb[i].kernel_stack_top = kernel_stack_top - i * STK_MAX_KER_PROC_OCC;
		pcb[i].user_stack_top = user_stack_top - i * STK_MAX_USR_PROC_OCC;
		// pcb[i].user_stack_top = STK_VADDR_BASE;
		pcb[i].kernel_context.regs[29] = pcb[i].kernel_stack_top;
		pcb[i].kernel_context.regs[30] = pcb[i].kernel_stack_top;
		pcb[i].user_context.regs[29] = pcb[i].user_stack_top;
		pcb[i].user_context.regs[30] = pcb[i].user_stack_top;
	}

	current_running = NULL;
	pcb_t *tmp;
	if ((tmp = create_proc_with_priority(0)) != NULL)
	{
		// this is the pcb of the kernel proc itself

		// do not push the kernel pcb into the ready_queue
		// set it as current_running directly
		current_running = tmp;
		tmp->type = KERNEL_PROCESS;
		tmp->name = "kernel";
	}

	task_info_t _shell = {"shell", shell, USER_PROCESS};
	do_spawn_with_priority(&_shell, 0, NULL);
}

static void init_exception_handler()
{
}

static void init_exception()
{
	// 1. Get CP0_STATUS
	// 2. Disable all interrupt
	// 3. Copy the level 2 exception handling code to 0x80000180
	// 4. reset CP0_COMPARE & CP0_COUNT register

	// note that interrupts are not enabled here.
	char *src = (char *)jmp_exception_handler_entry;
	char *dest = (char *)0x80000180;
	for (int i = 0; i < 256; ++i)
	{
		dest[i] = src[i];
	}
	src = jmp_TLB_exception_handler_entry;
	dest = 0x80000000;
	for (int i = 0; i < 180; ++i)
	{
		dest[i] = src[i];
	}

	// Clear all TLB
	// __asm__ volatile(
	//     "mtc0 $0, $10 \t\n" // cp0_hi
	//     "mtc0 $0, $3 \t\n"  // cp0_entrylo1
	//     "mtc0 $0, $2 \t\n"  // cp0_entrylo0
	//     "move $26, $0 \t\n" // $k0 as a loop variable
	//     "li   $27, 32 \t\n" // $k1 as upper bound
	//     "1:mtc0 $26, $0 \t\n"
	//     "tlbwi \t\n"
	//     "addiu $26, $26, 1 \t\n"
	//     "bne  $26, $27, 1b");
}

static void init_syscall(void)
{
	// init system call table.
	syscall[SYSCALL_SLEEP] = do_sleep;
	syscall[SYSCALL_BLOCK] = do_block;
	syscall[SYSCALL_UNBLOCK_ONE] = do_unblock_one;
	syscall[SYSCALL_UNBLOCK_ALL] = do_unblock_all;
	syscall[SYSCALL_WRITE] = screen_write;
	syscall[SYSCALL_CURSOR] = screen_move_cursor;
	syscall[SYSCALL_REFLUSH] = screen_reflush;
	syscall[SYSCALL_MUTEX_LOCK_INIT] = do_mutex_lock_init;
	syscall[SYSCALL_MUTEX_LOCK_ACQUIRE] = do_mutex_lock_acquire;
	syscall[SYSCALL_MUTEX_LOCK_RELEASE] = do_mutex_lock_release;
	syscall[SYSCALL_MUTEX_LOCK_RELEASE_MULTIPLE] = do_mutex_lock_release_multiple;
	syscall[SYSCALL_CLEAR] = screen_clear;
	syscall[SYSCALL_PS] = do_ps;
	syscall[SYSCALL_SPAWN] = do_spawn;
	syscall[SYSCALL_SPAWN_PRIORITY] = do_spawn_with_priority;
	syscall[SYSCALL_EXIT] = do_exit;
	syscall[SYSCALL_GET_PID] = do_get_pid;
	syscall[SYSCALL_WAIT] = do_waitpid;
	syscall[SYSCALL_EXEC] = do_exec;
	syscall[SYSCALL_KILL] = do_kill;
	syscall[SYSCALL_GETCHAR] = do_getchar;
	syscall[SYSCALL_SEMAPHORE_INIT] = do_semaphore_init;
	syscall[SYSCALL_SEMAPHORE_UP] = do_semaphore_up;
	syscall[SYSCALL_SEMAPHORE_DOWN] = do_semaphore_down;
	syscall[SYSCALL_COND_INIT] = do_condition_init;
	syscall[SYSCALL_COND_WAIT] = do_condition_wait;
	syscall[SYSCALL_COND_SIGNAL] = do_condition_signal;
	syscall[SYSCALL_COND_BROADCAST] = do_condition_broadcast;
	syscall[SYSCALL_BARRIER_INIT] = do_barrier_init;
	syscall[SYSCALL_BARRIER_WAIT] = do_barrier_wait;
	syscall[SYSCALL_REBOOT] = do_reboot;
	syscall[SYSCALL_MAC_INIT] = do_init_mac;
	syscall[SYSCALL_NET_SEND] = do_net_send;
	syscall[SYSCALL_NET_RECV] = do_net_recv;
	syscall[SYSCALL_NET_WAIT_RECV_PKG] = do_wait_recv_package;
	syscall[SYSCALL_MKFS] = do_mkfs;
	syscall[SYSCALL_FS_INFO] = do_print_fs_info;
	syscall[SYSCALL_READ_DIR] = do_ls;
	syscall[SYSCALL_MKDIR] = do_mkdir;
	syscall[SYSCALL_RMDIR] = do_rm;
	syscall[SYSCALL_CHANGE_DIR] = do_cd;
	syscall[SYSCALL_MKNOD] = do_touch;
	syscall[SYSCALL_CAT] = do_cat;
	syscall[SYSCALL_FOPEN] = do_fopen;
	syscall[SYSCALL_FREAD] = do_fread;
	syscall[SYSCALL_FWRITE] = do_fwrite;
	syscall[SYSCALL_FCLOSE] = do_fclose;
	syscall[SYSCALL_FSEEK] = do_fseek;
	syscall[SYSCALL_LINK] = do_link;
	syscall[SYSCALL_FIND] = do_find;
	syscall[SYSCALL_RENAME] = do_rename;
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
void __attribute__((section(".entry_function"))) _start(void)
{
	// Close the cache, no longer refresh the cache
	// when making the exception vector entry copy
	asm_start();

	// init interrupt (^_^)
	init_exception();
	printk("> [INIT] Interrupt processing initialization succeeded.\n");

	// init system call table (0_0)
	init_syscall();
	printk("> [INIT] System call initialized successfully.\n");

	// init priority ready queue
	init_pready_queue();
	printk("> [INIT] Priority ready queue initialized successfully.\n");

	// init Process Control Block (-_-!)
	init_pcb();
	printk("> [INIT] PCB initialization succeeded.\n");

	// init screen (QAQ)
	init_screen();
	printk("> [INIT] SCREEN initialization succeeded.\n");

	// Mount file system
	printk("> [INIT] Mount RUA-FS if it exists.\n");
	mount_fs();

	// init mem, including page table and TLB
	// do_mem_init();

	// Only needed in task 1
	// do_page_table_static_init();

	// TODO Enable interrupt
	enable_int();
	while (1)
	{
		// (QAQQQQQQQQQQQ)
		// If you do non-preemptive scheduling, you need to use it to surrender control
		// sys_exit();
#ifdef DEBUG
		// ptimer();
#endif
	}
	return;
}

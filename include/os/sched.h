/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking, 
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include "type.h"
#include "queue.h"
#include "mm.h"
#include "mac.h"
#include "fs.h"

#define NUM_MAX_TASK (16)

#define FIRST_PTE_NUM (1024)

#define INVALID_PID (-1)

#define STK_MAX_USR_PROC_OCC (16 * 1024)

#define STK_MAX_KER_PROC_OCC (16 * 1024)

#define __MAX_PRIORITY_NUM (5)

#define __PRIORITY_FACTOR (3)

#define __MAX_LOCK_NUM_FOR_A_PROC (8)

#define STK_UNMAPPED_BASE 0xa0f00000
// For task 1, where no TLB exception handler implemented
#define STK_VADDR_BASE (STK_MAX_KER_PROC_OCC * NUM_MAX_TASK + STK_UNMAPPED_BASE)
// #define STK_VADDR_BASE 0x40000000

/* used to save register infomation */
typedef struct regs_context
{
    /* Saved main processor registers.*/
    /* 32 * 4B = 128B */
    uint32_t regs[32];

    /* Saved special registers. */
    /* 7 * 4B = 28B */
    uint32_t cp0_status;
    uint32_t hi;
    uint32_t lo;
    uint32_t cp0_badvaddr;
    uint32_t cp0_cause;
    uint32_t cp0_epc;
    uint32_t pc;

} regs_context_t; /* 128 + 28 = 156B */

typedef enum
{
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_EXITED,
    TASK_CREATE
} task_status_t;

typedef enum
{
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    regs_context_t kernel_context;
    regs_context_t user_context;

    /* first level page table, at 312B */
    PTE_t *secondary_pt[FIRST_PTE_NUM];

    /* process id, at 4408B */
    pid_t pid;

    /* stack top of the process */
    uint32_t kernel_stack_top;
    uint32_t user_stack_top;

    /* name of the process */
    char *name;

    /* entry point of the proc */
    void *entry_point;

    /* previous, next pointer */
    void *prev;
    void *next;

    /* block queue for this proc */
    queue_t proc_block_queue;

    /* locks that acquired by this proc */
    void *locks[__MAX_LOCK_NUM_FOR_A_PROC];

    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | CREATE | EXITED */
    task_status_t status;

    /* sleeping counters and flags */
    uint32_t awake_time;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    /* priority of the proc, with 0 as the lowest priority */
    uint32_t priority;

    /* Receive descriptor */
    desc_t *waiting_desc;

    file_desc_t fd[1];
} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    char *name;
    uint32_t entry_point;
    task_type_t type;
} task_info_t;

// get the pcb ptr by its pid
pcb_t *get_proc_by_pid(pid_t pid);

/* ready queue to run */
extern queue_t ready_queue;

/* ready queue with priority */
extern queue_t priority_ready_queue[__MAX_PRIORITY_NUM];

/* priority queue execution counter */
extern uint32_t priority_ready_queue_counter[__MAX_PRIORITY_NUM];

/* priority queue execution counter upper bound */
extern uint32_t priority_ready_queue_counter_uppper_bound[__MAX_PRIORITY_NUM];

/* block queue to wait */
extern queue_t block_queue;

/* current running task PCB */
extern pcb_t *current_running;
// extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
extern uint32_t initial_cp0_status;

void do_scheduler(void);
void do_sleep(uint32_t);

void do_block(queue_t *blk_queue);
void do_unblock_one(queue_t *);
void do_unblock_all(queue_t *);

// print all running processes.
void do_ps();

void invoke_blocked_proc(pcb_t *proc);

void reset_pcb(pcb_t *p);
void reset_reg_context(regs_context_t *reg_p);
pcb_t *create_proc();
pcb_t *create_proc_with_priority(uint32_t priority);

// increase the priority upper bound of priority_ready_queue[priority]
void increase_priority_upper_bound(uint32_t priority);

// decrease the priority upper bound of priority_ready_queue[priority]
void decrease_priority_upper_bound(uint32_t priority);

// spawn a user proc according to the *task, with priority set as 1.
void do_spawn(task_info_t *task, arg_t *arg);

// start a user proc according to the *task and its priority
// 0 is the high-response priority. other priorities are
// executed more if they have higher values.
void do_spawn_with_priority(task_info_t *task, int priority, arg_t *arg);

// proc exit
void do_exit();

// get the pid of a proc by its entry point
void do_get_pid(void *, int *);

// wait until the proc of pid exits.
void do_waitpid(int);

// execute certain task
void do_exec(int, arg_t *);

// kill a proc by its pid
void do_kill(int);

// reboot
void do_reboot(void);

// copy contents across address space
// kernel <-> user

// copy contents from user address space to kernel space
// Only works if source user space is in a page frame
void copy_from_user(char *, char *, int, int);

// copy contents from kernel address space to user space
// Only works if target user space is in a page frame
void copy_to_user(char *, char *, int, int);

#endif
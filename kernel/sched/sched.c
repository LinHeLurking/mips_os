#include "lock.h"
#include "time.h"
#include "stdio.h"
#include "sched.h"
#include "queue.h"
#include "screen.h"
#include "string.h"
#include "type.h"
#include "test.h"
#include "mac.h"

pcb_t pcb[NUM_MAX_TASK]; // #define NUM_MAX_TASK 16

/* current running task PCB */
extern pcb_t *current_running;

/* global process id */
// pid_t process_id = 1;

/* last chosen queue's index */
int last_sel_index = -1;

/* ready queue to run */
queue_t ready_queue;

/* block queue to wait */
queue_t block_queue;

/* ready queue with priority */
queue_t priority_ready_queue[__MAX_PRIORITY_NUM];

int glb_sched_cnt = 0;
int glb_sched_bound = 5;

/* priority queue execution counter */
uint32_t priority_ready_queue_counter[__MAX_PRIORITY_NUM];

/* priority queue execution counter upper bound */
uint32_t priority_ready_queue_counter_uppper_bound[__MAX_PRIORITY_NUM];

/* kernel_context.regs[31] for all user process */
extern void exception_handler_exit();

// get the pcb ptr by its pid
pcb_t *get_proc_by_pid(pid_t pid)
{
    if (pid < 0 || pid >= NUM_MAX_TASK)
        return NULL;
    if (pcb[pid].status == TASK_RUNNING || pcb[pid].status == TASK_BLOCKED || pcb[pid].status == TASK_READY)
        return &pcb[pid];
    else
        return NULL;
}

static void check_sleeping()
{
    uint32_t cur_time = get_timer();
    pcb_t *blk_top = block_queue.head;
    if (blk_top != NULL)
    {
        if (cur_time > blk_top->awake_time)
        {
            pcb_t *proc = (pcb_t *)queue_dequeue(&block_queue);
            proc->awake_time = 0;
            invoke_blocked_proc(proc);
        }
    }
}

static void check_net_recv()
{
    pcb_t *proc = recv_block_queue.head;
    if (proc != NULL)
    {
        desc_t *waiting_desc = proc->waiting_desc;
        if ((waiting_desc->tdes0 & 0x80000000) == 0)
        {
            queue_dequeue(&recv_block_queue);
            invoke_blocked_proc(proc);
        }
    }
}

static void do_print_proc_info(pcb_t *proc)
{
    int pid = proc->pid;
    int priority = proc->priority;
    char *status;
    char *name = proc->name;
    if (proc->status == TASK_READY)
    {
        status = "ready";
    }
    else if (proc->status == TASK_RUNNING)
    {
        status = "running";
    }
    else if (proc->status == TASK_EXITED)
    {
        status = "exited";
    }
    printks("PID: %d\tNAME: %s\tPRIORITY: %d\tSTATUS: %s\n", pid, name, priority, status);
}

// Copy contents from user address space to kernel space.
// Only works if source user space is in a page frame
void copy_from_user(char *src, char *dest, int src_pid, int size)
{
    PTE_t *pte = querry_PTE((uint32_t)src >> 12, src_pid);
    uint32_t paddr = (((pte->entry) & ~(0b111111)) << 6) | ((uint32_t)src & 0xfff);
    char *unmapped_addr = paddr | 0xa0000000;
    // printk("Src is 0x%x, unmapped addr is 0x%x\n", src, unmapped_addr);
    for (int i = 0; i < size; ++i)
    {
        dest[i] = unmapped_addr[i];
    }
}

// copy contents from kernel address space to user space
// Only works if target user space is in a page frame
void copy_to_user(char *src, char *dest, int dest_pid, int size)
{
    PTE_t *pte = querry_PTE((uint32_t)dest >> 12, dest_pid);
    if (pte == NULL)
    {
        add_2_page((uint32_t)dest >> 13, dest_pid);
        pte = querry_PTE((uint32_t)dest >> 12, dest_pid);
    }
    uint32_t pfn = (pte->entry) >> 6;
    uint32_t paddr = (((pte->entry) & ~(0b111111)) << 6) | ((uint32_t)dest & 0xfff);
    char *unmapped_addr = paddr | 0xa0000000;
    // printk("Dest is 0x%x, unmapped addr is 0x%x\n", dest, unmapped_addr);
    for (int i = 0; i < size; ++i)
    {
        unmapped_addr[i] = src[i];
    }
}

// the internal part bwtween save and restore context
void scheduler(void)
{
    int this_index;
    if (last_sel_index >= __MAX_PRIORITY_NUM)
    {
        this_index = 1;
    }
    else
    {
        this_index = last_sel_index;
    }
    while (this_index < __MAX_PRIORITY_NUM && priority_ready_queue_counter[this_index] >= priority_ready_queue_counter_uppper_bound[this_index])
    {
        ++this_index;
    }
    if (this_index >= __MAX_PRIORITY_NUM)
    {
        this_index = 1;
        for (int i = 1; i < __MAX_PRIORITY_NUM; ++i)
        {
            priority_ready_queue_counter[i] = 0;
        }
    }

    if (++glb_sched_cnt >= glb_sched_bound)
    {
        this_index = 0;
        glb_sched_cnt = 0;
    }
    else if (priority_ready_queue[this_index].head == NULL)
    {
        this_index = 0;
    }
    else
    {
        last_sel_index = this_index;
        priority_ready_queue_counter[this_index] += 1;
    }

    queue_t *sel_queue = &priority_ready_queue[this_index];

    // Modify the current_running pointer.
    pcb_t *top = queue_dequeue(sel_queue);
    while (top != NULL && top->status == TASK_EXITED)
    {
        top = queue_dequeue(sel_queue);
    }

    if (top != NULL)
    {
        top->status = TASK_RUNNING;

        // save current screen cursor
        current_running->cursor_x = screen_cursor_x;
        current_running->cursor_y = screen_cursor_y;

        if (current_running->status != TASK_BLOCKED && current_running->status != TASK_EXITED)
        {
            queue_push(&priority_ready_queue[current_running->priority], current_running);
        }

        // awake sleeping proc if needed
        check_sleeping();

        // awake blocked net receiving proc if needed
        check_net_recv();

        current_running = top;
        // relocate screen cursor
        screen_cursor_x = current_running->cursor_x;
        screen_cursor_y = current_running->cursor_y;

        // // Reset asid in cp0_index
        // uint32_t index = current_running->pid;
        // __asm__ volatile(
        //     "mtc0 %0, $0" // cp0_index
        //     :
        //     : "r"(index));

        // Clear all TLB
        __asm__ volatile(
            "mtc0 $0, $10 \t\n" // cp0_hi
            "mtc0 $0, $3 \t\n"  // cp0_entrylo1
            "mtc0 $0, $2 \t\n"  // cp0_entrylo0
            "move $26, $0 \t\n" // $k0 as a loop variable
            "li   $27, 32 \t\n" // $k1 as upper bound
            "1:mtc0 $26, $0 \t\n"
            "tlbwi \t\n"
            "addiu $26, $26, 1 \t\n"
            "bne  $26, $27, 1b");
    }
    else
    {
        // nothing to do? so what?
        printk("[ERROR] scheduler error!\n");
    }
}

void do_sleep(uint32_t sleep_time)
{
    int awkt = sleep_time + get_timer();
    current_running->awake_time = awkt;
    do_block(&block_queue);
}

// block a proc oneself and push it into the blk_queue
void do_block(queue_t *blk_queue)
{
    // block the current_running proc into the queue
    queue_push_if_not_exist(blk_queue, current_running);
    current_running->status = TASK_BLOCKED;
    decrease_priority_upper_bound(current_running->priority);
    do_scheduler();
}

/* 
    Note that this function only extract the blocked proc
    and push it into the ready queue. This function does not
    really do_scheduler()
*/
void do_unblock_one(queue_t *queue)
{
    // unblock the head task from the queue
    pcb_t *proc = (pcb_t *)queue_dequeue(queue);
    // CUATIONS! You MUST check whether proc is NULL, or hidden errors occur.
    while (proc != NULL && proc->status == TASK_EXITED)
    {
        proc = (pcb_t *)queue_dequeue(queue);
    }
    if (proc == NULL)
        return;
    invoke_blocked_proc(proc);
}

void do_unblock_all(queue_t *queue)
{
    // unblock all task in the queue
    while (queue->head != NULL)
    {
        do_unblock_one(queue);
    }
}

void reset_reg_context(regs_context_t *reg_p)
{
    for (int i = 0; i < 32; ++i)
    {
        reg_p->regs[i] = 0;
    }
    reg_p->cp0_badvaddr = reg_p->cp0_cause = reg_p->cp0_status = 0;
    reg_p->hi = reg_p->lo = 0;
    reg_p->pc = 0;
}

void reset_pcb(pcb_t *p)
{
    p->kernel_stack_top = p->user_stack_top = 0;
    p->cursor_x = p->cursor_x = 0;
    p->type = USER_PROCESS;
    p->status = TASK_CREATE;
    p->pid = 0;
    p->awake_time = -1;
    p->prev = p->next = NULL;
    p->name = "unkown";
    p->entry_point = NULL;
    p->proc_block_queue.head = p->proc_block_queue.tail = NULL;
    p->priority = 1;
    // clear all lock status
    for (int i = 0; i < __MAX_LOCK_NUM_FOR_A_PROC; ++i)
    {
        p->locks[i] = NULL;
    }
    // reset_pcb() is executed after do_mem_init().
    // // clear all PTE
    // for (int i = 0; i < FIRST_PTE_NUM; ++i)
    // {
    //     p->secondary_pt[i] = NULL;
    // }
    reset_reg_context(&p->kernel_context);
    reset_reg_context(&p->user_context);
}

// traverse the pcb table to find an empty pcb and occupy it
// return NULL if find nothing available
pcb_t *create_proc()
{
    return create_proc_with_priority(1);
}

// cread a process with priority
pcb_t *create_proc_with_priority(uint32_t priority)
{
    if (priority >= __MAX_PRIORITY_NUM)
    {
        priority = __MAX_PRIORITY_NUM - 1;
    }
    else if (priority < 0)
    {
        priority = 0;
    }
    pcb_t *ret = NULL;
    for (int i = 0; i < NUM_MAX_TASK; ++i)
    {
        if (pcb[i].status == TASK_CREATE || pcb[i].status == TASK_EXITED)
        {
            ret = &pcb[i];
            ret->pid = i;
            ret->status = TASK_READY;
            ret->priority = priority;
            // ret->user_stack_top = STK_VADDR_BASE;
            ret->kernel_context.regs[29] = ret->kernel_context.regs[30] = ret->kernel_stack_top;
            ret->user_context.regs[29] = ret->user_context.regs[30] = ret->user_stack_top;
            break;
        }
    }
    increase_priority_upper_bound(priority);
    return ret;
}

// wake the blocked proc up
void invoke_blocked_proc(pcb_t *proc)
{
    // at the time you call this function, the blocked proc has
    // already been popped from the lock's waiting queue

    if (proc == NULL)
        return;

    // add it into the ready queue
    proc->status = TASK_RUNNING;
    increase_priority_upper_bound(proc->priority);
    queue_push_if_not_exist(&priority_ready_queue[proc->priority], (void *)proc);
}

// increase the priority upper bound of priority_ready_queue[priority]
void increase_priority_upper_bound(uint32_t priority)
{
    if (priority >= __MAX_PRIORITY_NUM)
    {
        priority = __MAX_PRIORITY_NUM - 1;
    }
    else if (priority < 0)
    {
        priority = 0;
    }
    priority_ready_queue_counter_uppper_bound[priority] += __PRIORITY_FACTOR * (priority + 1);
}

// decrease the priority upper bound of priority_ready_queue[priority]
void decrease_priority_upper_bound(uint32_t priority)
{
    if (priority >= __MAX_PRIORITY_NUM)
    {
        priority = __MAX_PRIORITY_NUM - 1;
    }
    else if (priority < 0)
    {
        priority = 0;
    }
    int decreament = __PRIORITY_FACTOR * (priority + 1);
    if (decreament > priority_ready_queue_counter_uppper_bound[priority])
    {
        priority_ready_queue_counter_uppper_bound[priority] = 0;
    }
    else
    {
        priority_ready_queue_counter_uppper_bound[priority] -= decreament;
    }
}

// print all running processes.
void do_ps()
{
    int cnt = 0;
    printks("[PROCESS TABLE]\n");
    printks("[%d] ", cnt++);
    do_print_proc_info(current_running);
    for (int i = 0; i < __MAX_PRIORITY_NUM; ++i)
    {
        queue_t *this_q = &priority_ready_queue[i];
        pcb_t *cur = this_q->head;
        while (cur != NULL)
        {
            printks("[%d] ", cnt++);
            do_print_proc_info(cur);
            cur = cur->next;
        }
    }
}

// start a user proc according to the *task and its priority
// 0 is the high-response priority. other priorities are
// executed more if they have higher values.
void do_spawn_with_priority(task_info_t *task, int priority, arg_t *arg)
{
    pcb_t *tmp;
    int argc;
    char **argv;
    if (arg == NULL)
    {
        argc = 0;
        argv = NULL;
    }
    else
    {
        argc = arg->argc;
        argv = arg->argv;
    }

    if ((tmp = create_proc_with_priority(priority)) != NULL)
    {
        queue_push(&priority_ready_queue[priority], (void *)tmp);
        tmp->entry_point = task->entry_point;
        tmp->type = task->type;
        tmp->kernel_context.regs[31] = (uint32_t)exception_handler_exit;
        tmp->user_context.cp0_epc = (uint32_t)task->entry_point;
        tmp->name = task->name;
        tmp->cursor_x = screen_cursor_x;
        tmp->cursor_y = screen_cursor_y;

        // if (argc > 0 && argv != NULL)
        // {
        //     // TODO: put argc and argv into the parameter slots of the new function
        //     // Note that here char *argv[] points to another proc(shell)'s space.
        //     // So, some copy operation is needed.

        //     char buff[16];
        //     char *new_argv[16];

        //     for (int i = 0; i < argc; ++i)
        //     {
        //         int len = strlen(argv[i]);
        //         tmp->user_stack_top -= len + 1;
        //         // strcpy(tmp->user_stack_top, argv[i]);
        //         copy_from_user(argv[i], buff, current_running->pid, len + 1);
        //         copy_to_user(buff, tmp->user_stack_top, tmp->pid, len + 1);
        //         new_argv[i] = tmp->user_stack_top;
        //         // argv[i][len] = '\0';
        //     }
        //     tmp->user_stack_top &= ~(0b11);
        //     for (int i = argc - 1; i >= 0; --i)
        //     {
        //         tmp->user_stack_top -= 4;
        //         // *(int *)tmp->user_stack_top = argv[i];
        //         copy_to_user(&new_argv[i], tmp->user_stack_top, tmp->pid, sizeof(char *));
        //     }
        //     void *ve = tmp->user_stack_top;
        //     tmp->user_stack_top -= 4;
        //     // *(int *)tmp->user_stack_top = ve;
        //     copy_to_user(&ve, tmp->user_stack_top, tmp->pid, sizeof(char *));
        //     tmp->user_stack_top -= 4;
        //     // *(int *)tmp->user_stack_top = argc;
        //     copy_to_user(&argc, tmp->user_stack_top, tmp->pid, sizeof(char *));
        //     // tmp->user_stack_top += 8;
        //     tmp->user_context.regs[4] = argc; // $a0
        //     tmp->user_context.regs[5] = ve;   // $a1
        //     tmp->user_context.regs[29] = tmp->user_context.regs[30] = tmp->user_stack_top;
        // }
    }
    else
    {
        // init error
    }
}

// spawn a user proc according to the *task, with priority set as 1.
void do_spawn(task_info_t *task, arg_t *arg)
{

    do_spawn_with_priority(task, 1, arg);
}

// proc exit
void do_exit()
{
    // do not use reset_pcb because it would clear the locks[] array
    // reset_pcb(current_running);
    reset_reg_context(&current_running->user_context.regs);
    reset_reg_context(&current_running->kernel_context.regs);
    current_running->awake_time = -1;
    current_running->kernel_context.regs[29] = current_running->kernel_context.regs[30] = current_running->kernel_stack_top;
    current_running->user_context.regs[29] = current_running->user_context.regs[30] = current_running->user_stack_top;
    current_running->status = TASK_EXITED;
    current_running->entry_point = NULL;
    current_running->name = "unknown";
    current_running->priority = 1;

    // release all locks
    for (int i = 0; i < __MAX_LOCK_NUM_FOR_A_PROC; ++i)
    {
        if (current_running->locks[i] != NULL)
        {
            do_mutex_lock_release(current_running->locks[i]);
            current_running->locks[i] = NULL;
        }
    }
    // invoke all processes that are waiting current_running exit
    pcb_t *cur = (pcb_t *)queue_dequeue(&current_running->proc_block_queue);
    while (cur != NULL)
    {
        if (cur->status != TASK_EXITED)
        {
            cur->status = TASK_RUNNING;
            // do not push to ready queue directly, invoke_blocked_proc contains some priority maintainance
            invoke_blocked_proc(cur);
        }
        cur = (pcb_t *)queue_dequeue(&current_running->proc_block_queue);
    }
    decrease_priority_upper_bound(current_running->priority);
    screen_reflush();
    do_scheduler();
}

// get the pid of a proc by its entry point
void do_get_pid(void *entry_point, int *ret)
{
    *ret = INVALID_PID;
    for (int i = 0; i < NUM_MAX_TASK; ++i)
    {
        if (pcb[i].entry_point == entry_point)
        {
            *ret = pcb[i].pid;
            break;
        }
    }
}

// wait until the proc of pid exits.
void do_waitpid(int pid)
{
    if (pid <= 0 || pid > NUM_MAX_TASK)
        return;
    pcb_t *proc = get_proc_by_pid(pid);
    if (proc != NULL)
        do_block(&proc->proc_block_queue);
}

// execute certain task
void do_exec(int task_id, arg_t *arg)
{
    if (task_id < 0 || task_id >= num_test_tasks)
        return;
    do_spawn(test_tasks[task_id], arg);
}

// kill a proc by its pid
void do_kill(int pid)
{
    // clear info and release stack space
    pcb_t *proc = get_proc_by_pid(pid);
    if (proc == NULL)
    {
        printks("Invalid pid.\nDo nothing.\n");
        return;
    }
    else if (proc->type == KERNEL_PROCESS)
    {
        printks("Cannot kill a kernel process.\n");
        return;
    }
    printks("Kill process \"%s\" whose pid = %d.\n", proc->name, proc->pid);
    reset_reg_context(&proc->kernel_context.regs);
    reset_reg_context(&proc->user_context.regs);
    proc->awake_time = -1;
    proc->kernel_context.regs[29] = proc->kernel_context.regs[30] = proc->kernel_stack_top;
    proc->user_context.regs[29] = proc->user_context.regs[30] = proc->user_stack_top;
    proc->status = TASK_EXITED;
    proc->entry_point = NULL;
    proc->name = "unknown";
    proc->priority = 1;

    // release all locks
    for (int i = 0; i < __MAX_LOCK_NUM_FOR_A_PROC; ++i)
    {
        if (proc->locks[i] != NULL)
        {
            do_mutex_lock_release(proc->locks[i]);
            proc->locks[i] = NULL;
        }
    }
    // invoke all processes that are blocked by this proc
    pcb_t *cur = (pcb_t *)queue_dequeue(&proc->proc_block_queue);
    while (cur != NULL)
    {
        if (cur->status != TASK_EXITED)
        {
            cur->status = TASK_RUNNING;
            invoke_blocked_proc(cur);
        }
        cur = (pcb_t *)queue_dequeue(&proc->proc_block_queue);
    }
    decrease_priority_upper_bound(proc->priority);
    screen_reflush();
}

#include "lock.h"
#include "sched.h"
#include "syscall.h"
#include "algo.h"

uint32_t lock_cnt = 0;

void spin_lock_init(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
}

void spin_lock_acquire(spin_lock_t *lock)
{
    while (LOCKED == lock->status)
    {
    }
    lock->status = LOCKED;
}

void spin_lock_release(spin_lock_t *lock)
{
    lock->status = UNLOCKED;
}

void do_mutex_lock_init(mutex_lock_t *lock)
{
    if (MUTEX_INITIALIZED != lock->is_init)
    {
        lock->status = UNLOCKED;
        lock->waiting_queue.head = lock->waiting_queue.tail = NULL;
        lock->lock_id = lock_cnt++;
        lock->is_init = MUTEX_INITIALIZED;
    }
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    while (lock->status == LOCKED)
    {
        queue_push_if_not_exist(&lock->waiting_queue, current_running);
        do_block(&lock->waiting_queue);
    }
    if (lock->status == UNLOCKED)
    {
        lock->status = LOCKED;
        for (int i = 0; i < __MAX_LOCK_NUM_FOR_A_PROC; ++i)
        {
            if (current_running->locks[i] == NULL)
            {
                current_running->locks[i] = lock;
                break;
            }
        }
        if (lock->waiting_queue.head == current_running)
        {
            queue_dequeue(&lock->waiting_queue);
        }
    }
}

// acquire multiple mutex locks
void mutex_lock_acquire_multiple(mutex_lock_t *locks[], int num)
{
    qsort(locks, 0, num - 1, cmp_lock);
    for (int i = 0; i < num; ++i)
    {
        mutex_lock_acquire(locks[i]);
    }
}

// release multiple mutex locks
void do_mutex_lock_release_multiple(mutex_lock_t *locks[], int num)
{
    qsort(locks, 0, num - 1, cmp_lock);
    for (int i = 0; i < num; ++i)
    {
        mutex_lock_t *lock = locks[i];
        lock->status = UNLOCKED;
        pcb_t *proc = (pcb_t *)queue_dequeue(&lock->waiting_queue);
        for (int i = 0; i < __MAX_LOCK_NUM_FOR_A_PROC; ++i)
        {
            if (current_running->locks[i] == lock)
            {
                current_running->locks[i] = NULL;
                break;
            }
        }
        invoke_blocked_proc(proc);
        // do_scheduler();
    }
    do_scheduler();
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    lock->status = UNLOCKED;
    for (int i = 0; i < __MAX_LOCK_NUM_FOR_A_PROC; ++i)
    {
        if (current_running->locks[i] == lock)
        {
            current_running->locks[i] = NULL;
            break;
        }
    }
    do_unblock_one(&lock->waiting_queue);
}

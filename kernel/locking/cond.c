#include "cond.h"
#include "lock.h"
#include "sched.h"

void do_condition_init(condition_t *condition)
{
    if (condition->initialized != 0)
        return;
    queue_init(&condition->cond_block_queue);
}

void do_condition_wait(mutex_lock_t *lock, condition_t *condition)
{
    do_mutex_lock_release(lock);
    // there's a do_scheduler() in do_block
    do_block(&condition->cond_block_queue);
    do_mutex_lock_acquire(lock);
}

void do_condition_signal(condition_t *condition)
{
    do_unblock_one(&condition->cond_block_queue);
}

void do_condition_broadcast(condition_t *condition)
{
    do_unblock_all(&condition->cond_block_queue);
}
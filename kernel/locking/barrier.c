#include "barrier.h"
#include "sched.h"

void do_barrier_init(barrier_t *barrier, int goal)
{
    if (barrier->initialized != 0)
        return;
    barrier->cur_val = 0;
    barrier->bound = goal;
}

void do_barrier_wait(barrier_t *barrier)
{
    barrier->cur_val += 1;
    if (barrier->cur_val < barrier->bound)
    {
        do_block(&barrier->barrier_block_queue);
    }
    else
    {
        barrier->cur_val = 0;
        do_unblock_all(&barrier->barrier_block_queue);
    }
}
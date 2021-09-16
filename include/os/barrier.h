#ifndef INCLUDE_BARRIER_H_
#define INCLUDE_BARRIER_H_

#include "queue.h"

typedef struct barrier
{
    int initialized;
    int cur_val;
    int bound;
    queue_t barrier_block_queue;
} barrier_t;

void do_barrier_init(barrier_t *, int);
void do_barrier_wait(barrier_t *);

#endif
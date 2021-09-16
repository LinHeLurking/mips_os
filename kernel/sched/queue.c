#include "queue.h"
#include "sched.h"

typedef pcb_t item_t;

// initialize the queue
void queue_init(queue_t *queue)
{
    queue->head = queue->tail = NULL;
}

// check if the queue is empty
int queue_is_empty(queue_t *queue)
{
    if (queue->head == NULL)
    {
        return 1;
    }
    return 0;
}

// push an element into the queue
void queue_push(queue_t *queue, void *item)
{
    item_t *_item = (item_t *)item;
    /* queue is empty */
    if (queue->head == NULL)
    {
        queue->head = item;
        queue->tail = item;
        _item->next = NULL;
        _item->prev = NULL;
    }
    else
    {
        ((item_t *)(queue->tail))->next = item;
        _item->next = NULL;
        _item->prev = queue->tail;
        queue->tail = item;
    }
}

// push an item into the queue if it does not exist in the queue currently
void queue_push_if_not_exist(queue_t *queue, void *item)
{
    queue_t *head = queue->head;
    while (head != NULL)
    {
        if (head == item)
        {
            return;
        }
        head = ((item_t *)head)->next;
    }
    queue_push(queue, item);
}

// pop the first item in the queue
void *queue_dequeue(queue_t *queue)
{
    item_t *temp = (item_t *)queue->head;

    if (temp == NULL)
        return temp;

    /* this queue only has one item */
    if (temp->next == NULL)
    {
        queue->head = queue->tail = NULL;
    }
    else
    {
        queue->head = ((item_t *)(queue->head))->next;
        ((item_t *)(queue->head))->prev = NULL;
    }

    temp->prev = NULL;
    temp->next = NULL;

    return (void *)temp;
}

/* remove this item and return next item */
void *queue_remove(queue_t *queue, void *item)
{
    item_t *_item = (item_t *)item;
    item_t *next = (item_t *)_item->next;

    if (item == queue->head && item == queue->tail)
    {
        queue->head = NULL;
        queue->tail = NULL;
    }
    else if (item == queue->head)
    {
        queue->head = _item->next;
        ((item_t *)(queue->head))->prev = NULL;
    }
    else if (item == queue->tail)
    {
        queue->tail = _item->prev;
        ((item_t *)(queue->tail))->next = NULL;
    }
    else
    {
        ((item_t *)(_item->prev))->next = _item->next;
        ((item_t *)(pcb->next))->prev = _item->prev;
    }

    _item->prev = NULL;
    _item->next = NULL;

    return (void *)next;
}

// return the top item of the queue without poppint it
void *queue_top(queue_t *queue)
{
    return queue->head;
}

// insert an element into the queue at the head
void queue_insert_at_head(queue_t *queue, void *item)
{
    item_t *old_hd = queue->head;
    if (old_hd == NULL)
    {
        queue->head = (item_t *)item;
    }
    else
    {
        ((item_t *)item)->next = old_hd;
        queue->head = (item_t *)item;
    }
}


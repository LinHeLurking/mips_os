#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#include "queue.h"
#include "lock.h"
#include "sem.h"

#define MAILBOX_MSG_MAX_LEN 64
#define MAILBOX_MSG_MAX_NUM 2
#define MAILBOX_MAX_NUM 8

typedef struct mailbox
{
    char msg[MAILBOX_MSG_MAX_NUM][MAILBOX_MSG_MAX_LEN];
    int msg_cnt;
    int head, tail;
    mutex_lock_t mailbox_lock;
    semaphore_t mailbox_empty;
    semaphore_t mailbox_full;
    char *name;
    int valid;
} mailbox_t;

mailbox_t _mailboxes[MAILBOX_MAX_NUM];

void mbox_init();
mailbox_t *mbox_open(char *);
void mbox_close(mailbox_t *);
void mbox_send(mailbox_t *, void *, int);
void mbox_recv(mailbox_t *, void *, int);

// hash function of const char*
uint32_t str_hash(char *);

#endif
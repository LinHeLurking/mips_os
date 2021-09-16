#include "string.h"
#include "mailbox.h"
#include "syscall.h"

#define MAX_NUM_BOX 32

static mailbox_t mboxs[MAX_NUM_BOX];

void mbox_init()
{
    for (int i = 0; i < MAILBOX_MAX_NUM; ++i)
    {
        for (int j = 0; j < MAILBOX_MSG_MAX_LEN; ++j)
        {
            for (int k = 0; k < MAILBOX_MSG_MAX_NUM; ++k)
            {
                _mailboxes[i].msg[k][j] = '\0';
            }
        }
        _mailboxes[i].valid = 0;
        _mailboxes[i].name = NULL;
        _mailboxes[i].head = _mailboxes[i].tail = 0;
        mutex_lock_init(&_mailboxes[i].mailbox_lock);
        semaphore_init(&_mailboxes[i].mailbox_empty, MAILBOX_MSG_MAX_NUM);
        semaphore_init(&_mailboxes[i].mailbox_full, 0);
    }
}

mailbox_t *mbox_open(char *name)
{
    // find if it exists
    for (int i = 0; i < MAILBOX_MAX_NUM; ++i)
    {
        if (_mailboxes[i].valid == 0)
            continue;
        if (strcmp(name, _mailboxes[i].name) == 0)
        {
            return &_mailboxes[i];
        }
    }
    // no valid match, create one
    mailbox_t *mb = NULL;
    for (int i = 0; i < MAILBOX_MAX_NUM; ++i)
    {
        if (_mailboxes[i].valid == 0)
        {
            mb = &_mailboxes[i];
            break;
        }
    }
    if (mb != NULL)
    {
        mb->name = name;
        mb->valid = 1;
    }
    return mb;
}

void mbox_close(mailbox_t *mailbox)
{
    mailbox->valid = 0;
}

void mbox_send(mailbox_t *mailbox, void *msg, int msg_length)
{
    semaphore_down(&mailbox->mailbox_empty);
    mutex_lock_acquire(&mailbox->mailbox_lock);
    char *_msg = msg;
    mailbox->tail = (mailbox->tail + 1) % MAILBOX_MSG_MAX_NUM;
    int cnt = mailbox->tail;
    if (mailbox != NULL && msg_length <= MAILBOX_MSG_MAX_LEN)
    {
        for (int i = 0; i < msg_length; ++i)
        {
            mailbox->msg[cnt][i] = _msg[i];
        }
    }
    mutex_lock_release(&mailbox->mailbox_lock);
    semaphore_up(&mailbox->mailbox_full);
}

void mbox_recv(mailbox_t *mailbox, void *msg, int msg_length)
{
    semaphore_down(&mailbox->mailbox_full);
    mutex_lock_acquire(&mailbox->mailbox_lock);
    char *_msg = msg;
    mailbox->head = (mailbox->head + 1) % MAILBOX_MSG_MAX_NUM;
    int cnt = mailbox->head;
    if (mailbox != NULL && msg_length <= MAILBOX_MSG_MAX_LEN)
    {
        for (int i = 0; i < msg_length; ++i)
        {
            _msg[i] = mailbox->msg[cnt][i];
        }
    }
    mutex_lock_release(&mailbox->mailbox_lock);
    semaphore_up(&mailbox->mailbox_empty);
}

#include "lock.h"
#include "sched.h"
#include "common.h"
#include "screen.h"
#include "sem.h"
#include "syscall.h"

void system_call_helper(int fn, int arg1, int arg2, int arg3)
{
    syscall[fn](arg1, arg2, arg3);
}

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE);
}

void sys_block(queue_t *queue)
{
    invoke_syscall(SYSCALL_BLOCK, (int)queue, IGNORE, IGNORE);
}

void sys_unblock_one(queue_t *queue)
{
    invoke_syscall(SYSCALL_UNBLOCK_ONE, (int)queue, IGNORE, IGNORE);
}

void sys_unblock_all(queue_t *queue)
{
    invoke_syscall(SYSCALL_UNBLOCK_ALL, (int)queue, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, (int)buff, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

void sys_clear()
{
    invoke_syscall(SYSCALL_CLEAR, 0, SCREEN_HEIGHT, IGNORE);
}

// sys_move_cursor(x,y) moves the cursor to y-th row and x-th col
void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE);
}

void mutex_lock_init(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_INIT, (int)lock, IGNORE, IGNORE);
}

void mutex_lock_acquire(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_ACQUIRE, (int)lock, IGNORE, IGNORE);
}

void mutex_lock_release(mutex_lock_t *lock)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_RELEASE, (int)lock, IGNORE, IGNORE);
}

void mutex_lock_release_multiple(mutex_lock_t *locks[], int num)
{
    invoke_syscall(SYSCALL_MUTEX_LOCK_RELEASE_MULTIPLE, (int)locks, num, IGNORE);
}

// print all running processes
void sys_ps()
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE);
}

// process exit
void sys_exit()
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE);
}

void sys_spawn(task_info_t *task, arg_t *arg)
{
    invoke_syscall(SYSCALL_SPAWN, (int)task, (int)arg, IGNORE);
}

void sys_spawn_with_priority(task_info_t *task, int priority, arg_t *arg)
{
    invoke_syscall(SYSCALL_SPAWN_PRIORITY, (int)task, priority, (int)arg);
}

// get the pid of a proc by its entry point
void sys_get_pid(void *entry_point, int *result)
{
    invoke_syscall(SYSCALL_GET_PID, (int)entry_point, (int)result, IGNORE);
}

// wait until the proc of pid exits.
void sys_waitpid(int pid)
{
    invoke_syscall(SYSCALL_WAIT, pid, IGNORE, IGNORE);
}

// execute certain task
void sys_exec(int task_id, arg_t *arg)
{
    invoke_syscall(SYSCALL_EXEC, task_id, (int)arg, IGNORE);
}

// kill a proc by its pid
void sys_kill(int pid)
{
    invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE);
}

// read a character from input
void sys_getchar(char *s)
{
    invoke_syscall(SYSCALL_GETCHAR, (int)s, IGNORE, IGNORE);
}

// semaphre init
void semaphore_init(semaphore_t *sem, int val)
{
    invoke_syscall(SYSCALL_SEMAPHORE_INIT, (int)sem, val, IGNORE);
}

// semaphore up
void semaphore_up(semaphore_t *sem)
{
    invoke_syscall(SYSCALL_SEMAPHORE_UP, (int)sem, IGNORE, IGNORE);
}

// semaphore down
void semaphore_down(semaphore_t *sem)
{
    invoke_syscall(SYSCALL_SEMAPHORE_DOWN, (int)sem, IGNORE, IGNORE);
}

// conditional variable init
void condition_init(condition_t *cond)
{
    invoke_syscall(SYSCALL_COND_INIT, (int)cond, IGNORE, IGNORE);
}

// conditonal varialble wait
void condition_wait(mutex_lock_t *lock, condition_t *cond)
{
    invoke_syscall(SYSCALL_COND_WAIT, (int)lock, (int)cond, IGNORE);
}

// conditional varialble signal
void condition_signal(condition_t *cond)
{
    invoke_syscall(SYSCALL_COND_SIGNAL, (int)cond, IGNORE, IGNORE);
}

// conditional variabnle broadcast
void condition_broadcast(condition_t *cond)
{
    invoke_syscall(SYSCALL_COND_BROADCAST, (int)cond, IGNORE, IGNORE);
}

// barrier init
void barrier_init(barrier_t *barrier, int bound)
{
    invoke_syscall(SYSCALL_BARRIER_INIT, (int)barrier, bound, IGNORE);
}

// barrier wait
void barrier_wait(barrier_t *barrier)
{
    invoke_syscall(SYSCALL_BARRIER_WAIT, (int)barrier, IGNORE, IGNORE);
}

// get my own pid
pid_t sys_getpid()
{
    pid_t pid;
    sys_get_pid(current_running->entry_point, &pid);
    return pid;
}

// reboot
void sys_reboot()
{
    invoke_syscall(SYSCALL_REBOOT, IGNORE, IGNORE, IGNORE);
}

// network
void sys_init_mac()
{
    invoke_syscall(SYSCALL_MAC_INIT, IGNORE, IGNORE, IGNORE);
}

void sys_net_send(uint32_t td, uint32_t td_phy)
{
    invoke_syscall(SYSCALL_NET_SEND, td, td_phy, IGNORE);
}

uint32_t sys_net_recv(uint32_t rd, uint32_t rd_phy, uint32_t daddr)
{
    invoke_syscall(SYSCALL_NET_RECV, rd, rd_phy, daddr);
    return 0;
}

void sys_wait_recv_package(desc_t *waiting_desc)
{
    invoke_syscall(SYSCALL_NET_WAIT_RECV_PKG, (int)waiting_desc, IGNORE, IGNORE);
}

void sys_mkfs()
{
    invoke_syscall(SYSCALL_MKFS, IGNORE, IGNORE, IGNORE);
}

void sys_statfs()
{
    invoke_syscall(SYSCALL_FS_INFO, IGNORE, IGNORE, IGNORE);
}

void sys_ls(char *name)
{
    invoke_syscall(SYSCALL_READ_DIR, (int)name, IGNORE, IGNORE);
}

void sys_mkdir(char *name)
{
    invoke_syscall(SYSCALL_MKDIR, (int)name, IGNORE, IGNORE);
}

void sys_rm(char *name)
{
    invoke_syscall(SYSCALL_RMDIR, (int)name, IGNORE, IGNORE);
}

void sys_cd(char *name)
{
    invoke_syscall(SYSCALL_CHANGE_DIR, (int)name, IGNORE, IGNORE);
}

void sys_touch(char *name)
{
    invoke_syscall(SYSCALL_MKNOD, (int)name, IGNORE, IGNORE);
}

void sys_cat(char *name)
{
    invoke_syscall(SYSCALL_CAT, (int)name, IGNORE, IGNORE);
}

int sys_fopen(char *name, int access)
{
    invoke_syscall(SYSCALL_FOPEN, (int)name, access, IGNORE);
    return 0;
}
void sys_fwrite(int fd, char *buf, uint32_t sz)
{
    invoke_syscall(SYSCALL_FWRITE, fd, (int)buf, (int)sz);
}
void sys_fread(int fd, char *buf, uint32_t sz)
{
    invoke_syscall(SYSCALL_FREAD, fd, (int)buf, (int)sz);
}
void sys_fclose(int fd)
{
    invoke_syscall(SYSCALL_FCLOSE, fd, IGNORE, IGNORE);
}

void sys_fseek(int fd, uint32_t pos)
{
    invoke_syscall(SYSCALL_FSEEK, fd, IGNORE, IGNORE);
}

void sys_link(char *_master, char *_servant, uint32_t link_type)
{
    invoke_syscall(SYSCALL_LINK, (int)_master, (int)_servant, (int)link_type);
}

void sys_find(char *path, char *name)
{
    invoke_syscall(SYSCALL_FIND, (int)path, (int)name, IGNORE);
}

void sys_rename(char *_old, char *_new)
{
    invoke_syscall(SYSCALL_RENAME, (int)_old, (int)_new, IGNORE);
}
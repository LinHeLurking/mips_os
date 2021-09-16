/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
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

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include "type.h"
#include "sync.h"
#include "queue.h"
#include "sched.h"
#include "sem.h"
#include "cond.h"
#include "barrier.h"
#include "mac.h"

#define IGNORE 0
#define NUM_SYSCALLS 128

/* define */
#define SYSCALL_SLEEP 2

#define SYSCALL_BLOCK 10
#define SYSCALL_UNBLOCK_ONE 11
#define SYSCALL_UNBLOCK_ALL 12

#define SYSCALL_WRITE 20
#define SYSCALL_READ 21
#define SYSCALL_CURSOR 22
#define SYSCALL_REFLUSH 23

#define SYSCALL_MUTEX_LOCK_INIT 30
#define SYSCALL_MUTEX_LOCK_ACQUIRE 31
#define SYSCALL_MUTEX_LOCK_RELEASE 32
#define SYSCALL_MUTEX_LOCK_RELEASE_MULTIPLE 33

#define SYSCALL_CLEAR 34
#define SYSCALL_PS 35

#define SYSCALL_SPAWN 36
#define SYSCALL_SPAWN_PRIORITY 37
#define SYSCALL_EXIT 38

#define SYSCALL_GET_PID 39
#define SYSCALL_WAIT 40

#define SYSCALL_EXEC 41
#define SYSCALL_KILL 42

#define SYSCALL_GETCHAR 43

#define SYSCALL_SEMAPHORE_INIT 44
#define SYSCALL_SEMAPHORE_UP 45
#define SYSCALL_SEMAPHORE_DOWN 46

#define SYSCALL_COND_INIT 47
#define SYSCALL_COND_WAIT 48
#define SYSCALL_COND_SIGNAL 49
#define SYSCALL_COND_BROADCAST 50

#define SYSCALL_BARRIER_INIT 51
#define SYSCALL_BARRIER_WAIT 52

#define SYSCALL_REBOOT 53

#define SYSCALL_MAC_INIT 54
#define SYSCALL_NET_SEND 55
#define SYSCALL_NET_RECV 56
#define SYSCALL_NET_WAIT_RECV_PKG 57

#define SYSCALL_MKFS 58
#define SYSCALL_MKDIR 59
#define SYSCALL_RMDIR 60
#define SYSCALL_READ_DIR 61
#define SYSCALL_FS_INFO 62
#define SYSCALL_CHANGE_DIR 63
#define SYSCALL_MKNOD 64
#define SYSCALL_CAT 65

#define SYSCALL_FOPEN 66
#define SYSCALL_FREAD 67
#define SYSCALL_FWRITE 68
#define SYSCALL_FCLOSE 69
#define SYSCALL_FSEEK 70
#define SYSCALL_LINK 71
#define SYSCALL_FIND 72
#define SYSCALL_RENAME 73

/* syscall function pointer */
int (*syscall[NUM_SYSCALLS])();

void system_call_helper(int, int, int, int);
extern int invoke_syscall(int, int, int, int);

void sys_sleep(uint32_t);

void sys_block(queue_t *);
void sys_unblock_one(queue_t *);
void sys_unblock_all(queue_t *);

void sys_write(char *);

void sys_clear();

// sys_move_cursor(x,y) moves the cursor to y-th row and x-th col
void sys_move_cursor(int, int);

void sys_reflush();

// print all running processes
void sys_ps();

// process exit
void sys_exit();

// kill a proc by its pid
void sys_kill(int pid);

// get the pid of a proc by its entry point
void sys_get_pid(void *, int *);

// get my own pid
pid_t sys_getpid();

// wait until the proc of pid exits.
void sys_waitpid(int);

void sys_spawn(task_info_t *, arg_t *);

void sys_spawn_with_priority(task_info_t *, int, arg_t *);

// execute certain task
void sys_exec(int, arg_t *);

// read a character from input
void sys_getchar(char *);

// semaphore up
void semaphore_up(semaphore_t *);

// semaphore down
void semaphore_down(semaphore_t *);

// semaphre init
void semaphore_init(semaphore_t *, int);

// conditional variable init
void condition_init(condition_t *);

// conditonal varialble wait
void condition_wait(mutex_lock_t *, condition_t *);

// conditional varialble signal
void condition_signal(condition_t *);

// conditional variabnle broadcast
void condition_broadcast(condition_t *);

// barrier init
void barrier_init(barrier_t *, int);

// barrier wait
void barrier_wait(barrier_t *);

// reboot
void sys_reboot();

// network
void sys_init_mac();
void sys_net_send(uint32_t, uint32_t);
uint32_t sys_net_recv(uint32_t, uint32_t, uint32_t);
void sys_wait_recv_package(desc_t *);

void mutex_lock_init(mutex_lock_t *);
void mutex_lock_acquire(mutex_lock_t *);
void mutex_lock_release(mutex_lock_t *);

void mutex_lock_acquire_multiple(mutex_lock_t *locks[], int num);
void mutex_lock_release_multiple(mutex_lock_t *locks[], int num);

void sys_mkfs();
void sys_statfs();
void sys_ls(char *name);
void sys_mkdir(char *name);
void sys_rm(char *name);
void sys_cd(char *name);
void sys_touch(char *name);
void sys_cat(char *name);

int sys_fopen(char *name, int access);
void sys_fwrite(int fd, char *buf, uint32_t sz);
void sys_fread(int fd, char *buf, uint32_t sz);
void sys_fclose(int fd);
void sys_fseek(int fd, uint32_t pos);
void sys_link(char *_master, char *_servant, uint32_t link_type);
void sys_find(char *path, char *name);
void sys_rename(char *_old, char *_new);

#endif
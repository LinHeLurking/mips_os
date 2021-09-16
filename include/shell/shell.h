#ifndef __SHELL_H
#define __SHELL_H

#define COMMAND_BUFFER_SIZE 1024
#define COMMAND_FIELD_MAX_NUM 64
#define COMMAND_IMPL_NUM 17

#define SOLVE_COMMAND_ERROR (-1)
#define SOLVE_COMMAND_OK (1)

#define SHELL_USER "LinHe"

// initialize the shell
void shell_init();


// solve a command string starts at cmd with length of len.
int solve_cmd(char *cmd, int len);

typedef struct cmd_t
{
    char *name;
    int id;
    int (*func)(int, char **);
} cmd_t;

int cmd_exec(int argc, char *argv[]);
int cmd_kill(int argc, char *argv[]);
int cmd_clear(int argc, char *argv[]);
int cmd_echo(int argc, char *argv[]);
int cmd_ps(int argc, char *argv[]);
int cmd_reboot(int argc, char *argv[]);
int cmd_mkfs(int argc, char *argv[]);
int cmd_statfs(int argc, char *argv[]);
int cmd_ls(int argc, char *argv[]);
int cmd_mkdir(int argc, char *argv[]);
int cmd_cd(int argc, char *argv[]);
int cmd_rm(int argc, char *argv[]);
int cmd_touch(int argc, char *argv[]);
int cmd_cat(int argc, char *argv[]);
int cmd_ln(int argc, char *argv[]);
int cmd_find(int argc, char *argv[]);
int cmd_rename(int argc, char *argv[]);

void shell();

#endif
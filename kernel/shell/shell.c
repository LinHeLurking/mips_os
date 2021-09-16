#include "shell.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "sched.h"
#include "screen.h"
#include "input.h"
#include "fs.h"

cmd_t shell_cmd[COMMAND_IMPL_NUM] = {
    {"exec", 0, cmd_exec},
    {"kill", 1, cmd_kill},
    {"clear", 2, cmd_clear},
    {"echo", 3, cmd_echo},
    {"ps", 4, cmd_ps},
    {"reboot", 5, cmd_reboot},
    {"mkfs", 6, cmd_mkfs},
    {"statfs", 7, cmd_statfs},
    {"ls", 8, cmd_ls},
    {"mkdir", 9, cmd_mkdir},
    {"rm", 10, cmd_rm},
    {"cd", 11, cmd_cd},
    {"touch", 12, cmd_touch},
    {"cat", 13, cmd_cat},
    {"ln", 14, cmd_ln},
    {"find", 15, cmd_find},
    {"rename", 16, cmd_rename}};

static char read_uart_ch(void)
{
    char ch = 0;
    volatile unsigned char *read_port = (unsigned char *)(0xbfe48000 + 0x00);
    volatile unsigned char *stat_port = (unsigned char *)(0xbfe48000 + 0x05);

    while ((*stat_port & 0x01))
    {
        ch = *read_port;
    }
    return ch;
}

int cmd_reboot(int argc, char *argv[])
{
    if (argc == 1 && strcmp(argv[0], "reboot") == 0)
    {
        sys_reboot();
    }
}

int cmd_exec(int argc, char *argv[])
{
    // for (int i = 1; i < argc; ++i)
    // {
    //     int task_id = atoi(argv[i]);
    //     sys_exec(task_id);
    // }
    int task_id = atoi(argv[1]);
    arg_t arg;
    arg.argc = argc - 1;
    arg.argv = &argv[1];
    sys_exec(task_id, &arg);
    return SOLVE_COMMAND_OK;
}

int cmd_kill(int argc, char *argv[])
{
    int pid = atoi(argv[1]);
    if (pid < 0 || pid > NUM_MAX_TASK)
    {
        printf("Incorrect pid: %d\n", pid);
        return SOLVE_COMMAND_ERROR;
    }
    // printf("Kill process with pid = %d\n", pid);
    sys_kill(pid);
    return SOLVE_COMMAND_OK;
}

int cmd_mkfs(int argc, char *argv[])
{
    sys_mkfs();
}

int cmd_statfs(int argc, char *argv[])
{
    sys_statfs();
}

int cmd_mkdir(int argc, char *argv[])
{
    char *name = argv[1];
    sys_mkdir(name);
}

int cmd_cd(int argc, char *argv[])
{
    char *name = argv[1];
    sys_cd(name);
}

int cmd_rm(int argc, char *argv[])
{
    char *name = argv[1];
    sys_rm(name);
}

int cmd_touch(int argc, char *argv[])
{
    char *name = argv[1];
    sys_touch(name);
}

int cmd_cat(int argc, char *argv[])
{
    char *name = argv[1];
    sys_cat(name);
}
int cmd_ln(int argc, char *argv[])
{
    uint32_t link_type;
    char *master;
    char *servant;
    if (argc == 3)
    {
        link_type = HARD_LINK;
        master = argv[1];
        servant = argv[2];
    }
    else if (argc == 4)
    {
        char *swc = argv[1];
        master = argv[2];
        servant = argv[3];
        if (strcmp(swc, "-s") == 0)
        {
            link_type = SOFT_LINK;
        }
        else
        {
            goto cmd_ln_usage;
        }
    }
    else
    {
        goto cmd_ln_usage;
    }

    sys_link(master, servant, link_type);
    return;

cmd_ln_usage:
    printf("Usage: ln [-s] a b\n\t Create link from a to b\n\t-s means it is a soft link\n");
}

int cmd_find(int argc, char *argv[])
{
    char *path = argv[1];
    char *name = argv[2];
    sys_find(path, name);
}

int cmd_rename(int argc, char *argv[])
{
    char *_old = argv[1];
    char *_new = argv[2];
    sys_rename(_old, _new);
}

int cmd_clear(int argc, char *argv[])
{
    sys_clear();
    sys_move_cursor(0, SCREEN_HEIGHT - 15);
    for (int i = 0; i < (SCREEN_WIDTH - 7) / 2 - 2; ++i)
    {
        printf("-");
    }
    printf("COMMAND");
    for (int i = 0; i < (SCREEN_WIDTH - 7) / 2 - 2; ++i)
    {
        printf("-");
    }
    printf("\n");
}

int cmd_echo(int argc, char *argv[])
{
    if (argc == 2)
    {
        printf("%s\n", argv[1]);
    }
    else if (argc == 4)
    {

        if (strcmp(argv[2], ">") == 0)
        {
            // echo a > b
            int fd = sys_fopen(argv[3], O_RDWR);
            sys_fwrite(fd, argv[1], strlen(argv[1]));
            sys_fclose(fd);
        }
    }
}

int cmd_ps(int argc, char *argv[])
{
    sys_ps();
}

int cmd_ls(int argc, char *argv[])
{
    if (argc == 1)
    {
        sys_ls(NULL);
    }
    else if (argc == 2)
    {
        sys_ls(argv[1]);
    }
}

// initialize the shell
void shell_init()
{
    sys_move_cursor(0, SCREEN_HEIGHT - 20);
    for (int i = 0; i < (SCREEN_WIDTH - 7) / 2 - 2; ++i)
    {
        printf("-");
    }
    printf("COMMAND");
    for (int i = 0; i < (SCREEN_WIDTH - 7) / 2 - 2; ++i)
    {
        printf("-");
    }
    printf("\n");
    printf("[SHELL] Shell initialization ends.\n");
    printf("[SHELL] Shell would be clear soon.\n");
}

// solve a command string starts at cmd with length of len.
int solve_cmd(char *cmd, int len)
{
    char *field[COMMAND_FIELD_MAX_NUM];
    // split the command string inot several parts
    int field_cnt = 0;
    cmd[len] = '\0';
    int last_is_blank = 1;
    for (int i = 0; i < len; ++i)
    {
        if (cmd[i] == ' ')
        {
            cmd[i] = '\0';
            last_is_blank = 1;
        }
        else if (last_is_blank)
        {
            last_is_blank = 0;
            field[field_cnt++] = &cmd[i];
        }
    }
    if (field[0] == NULL)
        return SOLVE_COMMAND_ERROR;
    // run the command
    int which_cmd = -1;
    for (int i = 0; i < COMMAND_IMPL_NUM; ++i)
    {
        if (shell_cmd[i].name == NULL)
        {
            break;
        }
        if (strcmp(field[0], shell_cmd[i].name) == 0)
        {
            which_cmd = i;
            break;
        }
    }
    if (which_cmd == -1)
    {
        return SOLVE_COMMAND_ERROR;
    }
    shell_cmd[which_cmd].func(field_cnt, field);
    field[0] = NULL;
    return SOLVE_COMMAND_OK;
}

void shell()
{
    char command_buffer[COMMAND_BUFFER_SIZE];
    int cnt = 0;
    char backspace = 8;
    char qemu_backspace = 127;
    int result;
    shell_init();
    cmd_clear(0, NULL);
    printf(SHELL_USER "@RUAOS %s ", (char *)cwd_str);
    while (1)
    {
        // read command from UART port
        char ch = getchar();

        // TODO solve command
        if (ch == '\0')
        {
            continue;
        }
        else if (ch == '\n' || ch == '\r')
        {
            command_buffer[cnt] = '\n';
            printf("\n");
            result = solve_cmd(command_buffer, cnt);
            cnt = 0;
            printf(SHELL_USER "@RUAOS %s ", (char *)cwd_str);
        }
        // backspace
        else if (ch == backspace || ch == qemu_backspace)
        {
            if (0 != cnt)
            {
                cnt -= 1;
                sys_move_cursor(screen_cursor_x - 1, screen_cursor_y);
                printf(" ");
                sys_move_cursor(screen_cursor_x - 1, screen_cursor_y);
            }
        }
        else
        {
            command_buffer[cnt++] = ch;
            printf("%c", ch);
        }
    }
}

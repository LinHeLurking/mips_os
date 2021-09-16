#include "input.h"
#include "syscall.h"

// read char from input
void do_getchar(char *s)
{
    char ch = 0;
    volatile unsigned char *read_port = (unsigned char *)(0xbfe48000 + 0x00);
    volatile unsigned char *stat_port = (unsigned char *)(0xbfe48000 + 0x05);

    while ((*stat_port & 0x01))
    {
        ch = *read_port;
    }
    *s = ch;
}

char getchar()
{
    char ret;
    sys_getchar(&ret);
    return ret;
}
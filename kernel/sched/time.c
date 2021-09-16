#include "time.h"

long long tick_elapsed = 0;

long long _slice_elapsed = 0;

uint32_t time_elapsed = 0;

static int MHZ = 300;

uint32_t get_ticks()
{
    return tick_elapsed;
}

uint32_t get_timer()
{
    return time_elapsed;
}

uint32_t get_timer_in_sec()
{
    return get_timer() / 1000;
}

void latency(uint32_t time)
{
    uint32_t begin_time = get_timer();

    while (get_timer() - begin_time < time)
    {
    };
    return;
}

// read the cp0 status register
uint32_t read_cp0_count()
{
    uint32_t count;
    __asm__ volatile(
        "mfc0 $k0, $9 \n\t"
        "sw $k0, %0"
        :
        : "m"(count));
    return count;
}

void update_time_stamp()
{
    int tick_since_last_int = read_cp0_count();
    int ms_cnt = MHZ * 1000 / 2;
    time_elapsed += tick_since_last_int / ms_cnt;
}


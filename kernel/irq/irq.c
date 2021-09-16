#include "irq.h"
#include "time.h"
#include "sched.h"
#include "string.h"
#include "stdio.h"
#include "screen.h"

void (*mac_int_handler)() = NULL;

// set cp0 count and compare
extern void set_count_compare();

static void irq_timer()
{
    // TODO clock interrupt handler.
    // scheduler, time counter in here to do, emmmmmm maybe.
    //printk("TIMER INTERRUPT\n");
    screen_reflush();
    do_scheduler();
    update_time_stamp();
    set_count_compare();
}

static void irq_device()
{
    // printk("irq_device\n");
    // printk("irq_device\n");
    // printk("irq_device\n");
    // printk("irq_device\n");
    // printk("irq_device\n");
    uint32_t int_sr = *(uint32_t *)INT1_SR;
    // printk("INT1_SR: %x", int_sr);
    if ((int_sr & LS1C_MAC_IRQ) != 0)
    {
        if (mac_int_handler != NULL)
        {
            (*mac_int_handler)();
        }
        else
        {
            printk("[ERROR] Uinitialized mac irq handler");
        }
    }
}

void interrupt_helper(uint32_t status, uint32_t cause)
{
    // TODO interrupt handler.
    // Leve3 exception Handler.
    // read CP0 register to analyze the type of interrupt.
    uint32_t ip = ((cause & status) >> 8) & (0b11111111);
    // switch (ip)
    // {
    // case 8:
    //     irq_device();
    //     break;
    // case 128:
    //     irq_timer();
    //     break;

    // default:
    //     break;
    // }
    // if (ip != 128)
    // {
    //     printk("IP: 0x%x", ip);
    //     printk("IP: 0x%x", ip);
    //     printk("IP: 0x%x", ip);
    //     printk("IP: 0x%x", ip);
    //     printk("IP: 0x%x", ip);
    // }
    if ((ip & 128) != 0)
    {
        irq_timer();
    }
    if ((ip & 8) != 0)
    {
        irq_device();
    }
}

void other_exception_handler()
{
    // TODO other exception handler
}
#include "mac.h"
#include "irq.h"
#include "string.h"
#include "sched.h"

#define NUM_DMA_DESC 48
queue_t recv_block_queue;
uint32_t recv_flag[PNUM] = {0};
uint32_t ch_flag;
uint32_t mac_cnt = 0;
uint32_t reg_read_32(uint32_t addr)
{
    return *((uint32_t *)addr);
}
uint32_t read_register(uint32_t base, uint32_t offset)
{
    uint32_t addr = base + offset;
    uint32_t data;

    data = *(volatile uint32_t *)addr;
    return data;
}

void reg_write_32(uint32_t addr, uint32_t data)
{
    *((uint32_t *)addr) = data;
}

static void gmac_get_mac_addr(uint8_t *mac_addr)
{
    uint32_t addr;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0Low);
    mac_addr[0] = (addr >> 0) & 0x000000FF;
    mac_addr[1] = (addr >> 8) & 0x000000FF;
    mac_addr[2] = (addr >> 16) & 0x000000FF;
    mac_addr[3] = (addr >> 24) & 0x000000FF;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0High);
    mac_addr[4] = (addr >> 0) & 0x000000FF;
    mac_addr[5] = (addr >> 8) & 0x000000FF;
}

void print_tx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("send buffer mac->saddr=0x%x ", mac->saddr);
    printf("mac->saddr_phy=0x%x ", mac->saddr_phy);
    printf("send discrb mac->td_phy=0x%x\n", mac->td_phy);
#if 0
    desc_t *send=mac->td;
    for(i=0;i<mac->pnum;i++)
    {
        printf("send[%d].tdes0=0x%x ",i,send[i].tdes0);
        printf("send[%d].tdes1=0x%x ",i,send[i].tdes1);
        printf("send[%d].tdes2=0x%x ",i,send[i].tdes2);
        printf("send[%d].tdes3=0x%x ",i,send[i].tdes3);
    }
#endif
}

void print_rx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("recieve buffer add mac->daddr=0x%x ", mac->daddr);
    printf("mac->daddr_phy=0x%x ", mac->daddr_phy);
    printf("recieve discrb add mac->rd_phy=0x%x\n", mac->rd_phy);
    desc_t *recieve = (desc_t *)mac->rd;
#if 0
    for(i=0;i<mac->pnum;i++)
    {
        printf("recieve[%d].tdes0=0x%x ",i,recieve[i].tdes0);
        printf("recieve[%d].tdes1=0x%x ",i,recieve[i].tdes1);
        printf("recieve[%d].tdes2=0x%x ",i,recieve[i].tdes2);
        printf("recieve[%d].tdes3=0x%x\n",i,recieve[i].tdes3);
    }
#endif
}

static uint32_t printf_recv_buffer(uint32_t recv_buffer)
{
}

void mac_irq_handle_send()
{
}

void mac_irq_handle_recv()
{
    // printk("mac_irq_handle_recv\n");
    pcb_t *proc = queue_dequeue(&recv_block_queue);
    if (proc != NULL)
    {
        invoke_blocked_proc(proc);
    }
    clear_interrupt();
}

void irq_enable(int IRQn)
{

    *(uint32_t *)INT1_EN = LS1C_MAC_IRQ;
}

void mac_recv_handle(mac_t *test_mac)
{
}

static uint32_t printk_recv_buffer(uint32_t recv_buffer)
{
}

static void reverse(uint32_t *x)
{
    char buf[4];
    char *s = x;
    for (int i = 0; i < 4; ++i)
    {
        buf[i] = s[4 - i];
    }
    *x = *(uint32_t *)buf;
}

void set_sram_ctr()
{
    *((volatile unsigned int *)0xbfd00420) = 0x8000; /* ??????GMAC0 */
}
static void s_reset(mac_t *mac) //reset mac regs
{
    uint32_t time = 1000000;
    reg_write_32(mac->dma_addr, 0x01);

    while ((reg_read_32(mac->dma_addr) & 0x01))
    {
        reg_write_32(mac->dma_addr, 0x01);
        while (time)
        {
            time--;
        }
    };
}
void disable_interrupt_all(mac_t *mac)
{
    reg_write_32(mac->dma_addr + DmaInterrupt, DmaIntDisable);
    return;
}
void set_mac_addr(mac_t *mac)
{
    uint32_t data;
    uint8_t MacAddr[6] = {0x00, 0x55, 0x7b, 0xb5, 0x7d, 0xf7};
    uint32_t MacHigh = 0x40, MacLow = 0x44;
    data = (MacAddr[5] << 8) | MacAddr[4];
    reg_write_32(mac->mac_addr + MacHigh, data);
    data = (MacAddr[3] << 24) | (MacAddr[2] << 16) | (MacAddr[1] << 8) | MacAddr[0];
    reg_write_32(mac->mac_addr + MacLow, data);
}
uint32_t do_net_recv(uint32_t rd, uint32_t rd_phy, uint32_t daddr)
{
    // printk("rd: %x, rd_phy: %x\n", rd, rd_phy);
    reg_write_32(DMA_BASE_ADDR + 0xc, rd_phy);
    reg_write_32(GMAC_BASE_ADDR, reg_read_32(GMAC_BASE_ADDR) | 0x4);
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(GMAC_BASE_ADDR + 0x18) | 0x02200002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));
    volatile desc_t *cur = rd;
    int cnt = 0;
    while (1)
    {
        cur->tdes0 = 0x80000000;
        reg_write_32(DMA_BASE_ADDR + 0x8, 1);
        cur = (cur->tdes3) | 0xa0000000;
        if ((cur->tdes1 & (1 << 25)) != 0)
        {
            break;
        }
    };
    return 0;
}

void do_net_send(uint32_t td, uint32_t td_phy)
{
    reg_write_32(DMA_BASE_ADDR + 0x10, td_phy);

    // MAC rx/tx enable

    reg_write_32(GMAC_BASE_ADDR, reg_read_32(GMAC_BASE_ADDR) | 0x8);                    // enable MAC-TX
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02202000); //0x02202002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));
    volatile desc_t *cur = td;
    while (1)
    {
        cur->tdes0 = 0x80000000;
        // printk("DES0: %d  ", cur->tdes0);
        reg_write_32(DMA_BASE_ADDR + 0x4, 1);
        volatile uint32_t *flag = &(cur->tdes0);
        while (((*flag) & 0x80000000) != 0)
        {
            int __useless__ = 0;
        }
        // printk("DES0: %d\n", cur->tdes0);
        if ((cur->tdes1 & (1 << 25)) != 0)
        {
            break;
        }
        cur = (cur->tdes3) | 0xa0000000;
    };
}

void do_init_mac(void)
{
    mac_t test_mac;
    uint32_t i;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum

    set_sram_ctr(); /* ??????GMAC0 */
    s_reset(&test_mac);
    disable_interrupt_all(&test_mac);
    set_mac_addr(&test_mac);
    *(uint32_t *)INT1_CLR = 0xfffffff;
    *(uint32_t *)INT1_POL = 0xfffffff;
    *(uint32_t *)INT1_EDGE = 0;
}

void do_wait_recv_package(desc_t *waiting_desc)
{
    current_running->waiting_desc = waiting_desc;
    volatile uint32_t *flag = &(waiting_desc->tdes0);
    while (((*flag) & 0x80000000) != 0)
    {
        do_block(&recv_block_queue);
    }
}
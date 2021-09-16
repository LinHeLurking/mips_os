#include "mac.h"
#include "irq.h"
#include "type.h"
#include "screen.h"
#include "syscall.h"
#include "sched.h"
#include "test.h"

desc_t *send_desc;
desc_t *receive_desc;
uint32_t cnt = 1; //record the time of iqr_mac
//uint32_t buffer[PSIZE] = {0x00040045, 0x00000100, 0x5d911120, 0x0101a8c0, 0xfb0000e0, 0xe914e914, 0x00000801,0x45000400, 0x00010000, 0x2011915d, 0xc0a80101, 0xe00000fb, 0x14e914e9, 0x01080000};
uint32_t buffer[PSIZE] = {0xffffffff, 0x5500ffff, 0xf77db57d, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0004e914, 0x0000, 0x005e0001, 0x2300fb00, 0x84b7f28b, 0x00450008, 0x0000d400, 0x11ff0040, 0xa8c073d8, 0x00e00101, 0xe914fb00, 0x0801e914, 0x0000};

#define BUFFER_SIZE 1024

static void register_irq_handler(uint32_t num, void (*handler)())
{
    if (num == LS1C_MAC_IRQ)
    {
        mac_int_handler = handler;
    }
}

static uint32_t printf_recv_buffer(uint32_t recv_buffer)
{
    uint32_t *read_buffer = recv_buffer;
    // print contents
    char re[4];
    int cnt = 0;
    while (*read_buffer != 0)
    {
        char *x = read_buffer;
        for (int i = 0; i < 4; ++i)
        {
            re[i] = x[4 - i];
        }
        printf("%x ", *(uint32_t *)re);
        if ((++cnt & 0b111) == 0)
        {
            printf("\n");
        }
        read_buffer++;
    }
    return 1;
}

/**
 * Clears all the pending interrupts.
 * If the Dma status register is read then all the interrupts gets cleared
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 */
void clear_interrupt()
{
    uint32_t data;
    data = reg_read_32(0xbfe11000 + DmaStatus);
    reg_write_32(0xbfe11000 + DmaStatus, data);
}

desc_t descriptor_pool[64];
uint8_t r_buffer[64][BUFFER_SIZE];

static void mac_send_desc_init(mac_t *mac)
{
    for (int i = 0; i < mac->pnum; ++i)
    {
        descriptor_pool[i].tdes0 = 0x80000000;
        descriptor_pool[i].tdes1 = ((i == mac->pnum - 1 ? 0 : 1) << 31) |
                                   (1 << 30) |
                                   (1 << 29) |
                                   ((i == mac->pnum - 1 ? 1 : 0) << 25) |
                                   (1 << 24) |
                                   (sizeof(buffer));
        descriptor_pool[i].tdes2 = (uint32_t)buffer & 0x1fffffff;                                           // paddr
        descriptor_pool[i].tdes3 = (uint32_t)&descriptor_pool[i == mac->pnum - 1 ? 0 : i + 1] & 0x1fffffff; // next paddr
    }
    // reg_write_32(mac->dma_addr + 0x10, (uint32_t)&descriptor_pool[0] & 0x1fffffff);
    // reg_write_32(mac->mac_addr, 1 << 3);
    mac->saddr = buffer;
    mac->saddr_phy = (uint32_t)buffer & 0x1fffffff;
    mac->td = &descriptor_pool[0];
    mac->td_phy = (uint32_t)&descriptor_pool[0] & 0x1fffffff;
}

static void mac_recv_desc_init(mac_t *mac)
{
    for (int i = 0; i < mac->pnum; ++i)
    {
        descriptor_pool[i].tdes0 = 0x80000000;
        descriptor_pool[i].tdes1 = ((i == mac->pnum - 1 ? 0 : 1) << 31) |
                                   ((i == mac->pnum - 1 ? 1 : 0) << 25) |
                                   (1 << 24) |
                                   (BUFFER_SIZE);
        descriptor_pool[i].tdes2 = (uint32_t)r_buffer[i] & 0x1fffffff;
        descriptor_pool[i].tdes3 = (uint32_t)&descriptor_pool[i == mac->pnum - 1 ? 0 : i + 1] & 0x1fffffff;
    }
    mac->rd = &descriptor_pool[0];
    mac->rd_phy = (uint32_t)&descriptor_pool[0] & 0x1fffffff;
}

static void mii_dul_force(mac_t *mac)
{
    reg_write_32(mac->dma_addr, 0x80); //?s
                                       //   reg_write_32(mac->dma_addr, 0x400);
    uint32_t conf = 0xc800;            //0x0080cc00;

    // loopback, 100M
    reg_write_32(mac->mac_addr, reg_read_32(mac->mac_addr) | (conf) | (1 << 8));
    //enable recieve all
    reg_write_32(mac->mac_addr + 0x4, reg_read_32(mac->mac_addr + 0x4) | 0x80000001);
}

void dma_control_init(mac_t *mac, uint32_t init_value)
{
    reg_write_32(mac->dma_addr + DmaControl, init_value);
    return;
}

void mac_send_task()
{

    mac_t test_mac;
    uint32_t i;
    uint32_t print_location = 2;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum

    mac_send_desc_init(&test_mac);

    dma_control_init(&test_mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&test_mac);

    mii_dul_force(&test_mac);

    register_irq_handler(LS1C_MAC_IRQ, mac_irq_handle_send);

    irq_enable(LS1C_MAC_IRQ);
    sys_move_cursor(1, print_location);
    printf("> [MAC SEND TASK] start send package.               \n");

    uint32_t cnt = 0;
    i = 4;
    while (i > 0)
    {
        sys_net_send(test_mac.td, test_mac.td_phy);
        cnt += PNUM;
        sys_move_cursor(1, print_location);
        printf("> [MAC SEND TASK] totally send package %d !        \n", cnt);
        i--;
    }
    sys_exit();
}

void mac_recv_task()
{

    mac_t test_mac;
    uint32_t i;
    uint32_t ret;
    uint32_t print_location = 1;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum
    mac_recv_desc_init(&test_mac);

    dma_control_init(&test_mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&test_mac);

    mii_dul_force(&test_mac);

    register_irq_handler(LS1C_MAC_IRQ, mac_irq_handle_recv);

    irq_enable(LS1C_MAC_IRQ);

    queue_init(&recv_block_queue);
    sys_move_cursor(1, print_location);
    printf("> [MAC RECV TASK] start recv:                    \n");
    ret = sys_net_recv(test_mac.rd, test_mac.rd_phy, test_mac.daddr);

    ch_flag = 0;
    for (i = 0; i < PNUM; i++)
    {
        recv_flag[i] = 0;
    }

    uint32_t cnt = 0;
    volatile uint32_t *Recv_desc;
    Recv_desc = (volatile uint32_t *)(test_mac.rd + (PNUM - 1) * 16);
    //printf("(test_mac.rd 0x%x ,Recv_desc=0x%x,REDS0 0X%x\n", test_mac.rd, Recv_desc, *(Recv_desc));
    if (((*Recv_desc) & 0x80000000) == 0x80000000)
    {
        sys_move_cursor(1, print_location);
        printf("> [MAC RECV TASK] waiting receive package.\n");
        
        sys_wait_recv_package((desc_t *)Recv_desc);
    }
    // mac_recv_handle(&test_mac);
    printf("> [MAC RECV TASK] now received all package.\n");
    print_location = 4;
    sys_move_cursor(1, print_location);
    printf("> [MAC RECV TASK] print them in order.\n");
    ++print_location;
    desc_t *cur = test_mac.rd;
    while (1)
    {
        uint32_t recv_buff = (cur->tdes2) | 0xa0000000;
        sys_move_cursor(1, print_location);
        printf_recv_buffer(recv_buff);
        printf("\n");
        if ((cur->tdes1 & (1 << 25)) != 0)
        {
            break;
        }
        cur = (cur->tdes3) | 0xa0000000;
        sys_sleep(200);
    }
    printf("> [MAC RECV TASK] receive end.\n");

    sys_exit();
}

void mac_init_task()
{
    uint32_t print_location = 1;
    sys_move_cursor(1, print_location);
    printf("> [MAC INIT TASK] Waiting for MAC initialization .\n");
    sys_init_mac();
    sys_move_cursor(1, print_location);
    printf("> [MAC INIT TASK] MAC initialization succeeded.           \n");
    sys_exit();
}

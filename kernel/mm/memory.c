#include "mm.h"
#include "algo.h"
#include "sched.h"
#include "stdio.h"

#define FRAME_NUM 2048

//TODO:Finish memory management functions here refer to mm.h and add any functions you need.

// Data structure that maintains the empty physical page frames
// This is a stack
void *empty_frame[FRAME_NUM];
uint32_t top;

// Check if the umapped vaddr is illegal
static int paddr_ok(void *paddr)
{
    // TODO: finish verdict
    return 1;
}

// Get the virtual page number 2 of a vaddr
uint32_t get_vpn2(uint32_t vaddr)
{
    return (vaddr >> 13);
}

// Get the virtual page number of a vaddr
uint32_t get_vpn(uint32_t vaddr)
{
    return (vaddr >> 12);
}

// Get the physical frame number of a paddr
uint32_t get_pfn(uint32_t paddr)
{
    return (paddr >> 12);
}

// Get the start unmapped addr of a frame
void *get_frame_start(void *unmapped)
{
    return (uint32_t)unmapped & (~(PAGE_SIZE - 1));
}

// Get paddr from an unmapped addr
void *get_paddr(void *unmapped)
{
    return (void *)((uint32_t)unmapped & 0xffffff);
}

// Get the first level page table index from vpn
uint32_t get_first_level_index(uint32_t vpn)
{
    return (vpn >> 9);
}

// Get the second level page table index from vpn
uint32_t get_second_level_index(uint32_t vpn)
{
    return (vpn & 511);
}

// Search in current_running's page table
PTE_t *querry_PTE(uint32_t vpn, int pid)
{
#ifdef DEBUG
    // printk("Querring vpn: %x\n", vpn);
#endif
    pcb_t *proc = get_proc_by_pid(pid);
    PTE_t *second = proc->secondary_pt[get_first_level_index(vpn)];
    if (second != NULL)
    {
        return &second[get_second_level_index(vpn)];
    }
    return NULL;
}

// Only needed in task 1.
// Initialize some global page table entries.
void do_page_table_static_init()
{
    uint32_t base = 0x40000000;
    uint32_t unmapped_page_addr;
    uint32_t unmapped_frame_addr;
    uint32_t vpn;
    uint32_t pfn;
    uint32_t entry;
    uint32_t fst;
    uint32_t sec;

    uint32_t paddr;
    uint32_t frame_start;
    // even
    unmapped_page_addr = kalloc();  // use for storing the page
    unmapped_frame_addr = kalloc(); // the physical frame of that page
    vpn = get_vpn(base);
    frame_start = get_frame_start(unmapped_frame_addr);
    paddr = get_paddr(frame_start);
    pfn = get_pfn(paddr);
    entry = COMPRESS_ENTRYLO(pfn, 2, 1, 1, 1);
    fst = get_first_level_index(vpn);
    sec = get_second_level_index(vpn);
    current_running->secondary_pt[fst] = unmapped_page_addr;
    current_running->secondary_pt[fst][sec].vpn = vpn;
    current_running->secondary_pt[fst][sec].entry = entry;
    PTE_t *pte1 = querry_PTE(vpn, current_running);
    base += PAGE_SIZE;
    // odd
    unmapped_page_addr = kalloc();
    unmapped_frame_addr = kalloc();
    vpn = get_vpn(base);
    pfn = get_pfn(get_paddr(get_frame_start(unmapped_frame_addr)));
    entry = COMPRESS_ENTRYLO(pfn, 2, 1, 1, 1);
    fst = get_first_level_index(vpn);
    sec = get_second_level_index(vpn);
    current_running->secondary_pt[fst] = unmapped_page_addr;
    current_running->secondary_pt[fst][sec].vpn = vpn;
    current_running->secondary_pt[fst][sec].entry = entry;
    PTE_t *pte2 = querry_PTE(vpn, current_running);
    // write TLB
    write_TLB_index(pte1, pte2, 10);
}

// Iintialize the memory
void do_mem_init()
{
    uint32_t start = 0xa08f0000;
    for (int i = 0; i < FRAME_NUM; ++i)
    {
        empty_frame[i] = start;
        start += PAGE_SIZE;
    }
    top = 0;
}

// Write 2 continuous PTEs into TLB
void write_TLB_index(PTE_t *a, PTE_t *b, uint32_t index)
{
    uint32_t hi = ((a->vpn >> 1) << 13) | (current_running->pid & 0xff);
    uint32_t lo0 = a->entry;
    uint32_t lo1 = b->entry;
    __asm__ volatile(
        "lw   $k0, %0 \t\n"
        "mtc0 $k0, $10 \t\n" // entryhi
        "lw   $k0, %1 \t\n"
        "mtc0 $k0, $2 \t\n" // entrylo0
        "lw   $k0, %2 \t\n"
        "mtc0 $k0, $3 \t\n" // entrylo1
        "mtc0 $0, $5 \t\n"  // pagemask
        "mtc0 $a2, $0 \t\n" // index
        "tlbwi"
        :
        : "m"(hi), "m"(lo0), "m"(lo1));

#ifdef DEBUG
    uint32_t check_index;
    __asm__ volatile(
        "lw $k0, %1 \t\n"
        "tlbp \t\n"
        "mfc0 $k1, $0 \t\n"
        "sw $k1, %0"
        : "=m"(check_index)
        : "m"(hi));
    if (check_index & 0x80000000)
    {
        printk("Error in write_TLB_index()\n");
    }
    else if (check_index != index)
    {
        printk("Error in write_TLB_index()\n");
    }
    else
    {
        printk("TLB write correct.\n");
    }
#endif
}

// Write 2 continuous PTES into a random TLB
void write_TLB_rand(PTE_t *a, PTE_t *b)
{
    uint32_t hi = ((a->vpn >> 1) << 13) | (current_running->pid & 0xff);
    uint32_t lo0 = a->entry;
    uint32_t lo1 = b->entry;
    __asm__ volatile(
        "lw   $k0, %0 \t\n"
        "mtc0 $k0, $10 \t\n" // entryhi
        "lw   $k0, %1 \t\n"
        "mtc0 $k0, $2 \t\n" // entrylo0
        "lw   $k0, %2 \t\n"
        "mtc0 $k0, $3 \t\n" // entrylo1
        "mtc0 $0, $5 \t\n"  // pagemask
        "tlbwr"
        :
        : "m"(hi), "m"(lo0), "m"(lo1));
#ifdef DEBUG
    uint32_t check_index;
    __asm__ volatile(
        "lw $k0, %1 \t\n"
        "tlbp \t\n"
        "mfc0 $k1, $0 \t\n"
        "sw $k1, %0"
        : "=m"(check_index)
        : "m"(hi));
    if (check_index & 0x80000000)
    {
        printk("Error in write_TLB_rand()\n");
    }
    else
    {
        printk("TLB write rand correct.\n");
        printk("Entry hi: %x\n", hi);
        printk("Entry lo0: %x\n", lo0);
        printk("Entry lo1: %x\n", lo1);
    }
#endif
}

// Allocate a physical page frame, and return the starting unmapped addr.
void *kalloc()
{
    if (top < FRAME_NUM - 1)
    {
        int *addr = empty_frame[top];
        for (int i = 0; i < PAGE_SIZE / 4; ++i)
        {
            addr[i] = 0;
        }
        return empty_frame[top++];
    }
    else
    {
        // Report an error?
    }
}

// Free a physical page fram
void kfree(void *paddr)
{
    if (paddr_ok(paddr) && top > 0)
    {
        empty_frame[--top] = paddr;
    }
    else
    {
        // Report an error?
    }
}

// Add 2 continuous virtual pages at once
void add_2_page(uint32_t vpn2, int pid)
{
    uint32_t base = vpn2 << 13;
    uint32_t unmapped_page_addr;
    uint32_t unmapped_frame_addr;
    uint32_t vpn;
    uint32_t pfn;
    uint32_t entry;
    uint32_t fst;
    uint32_t sec;
    pcb_t *proc = get_proc_by_pid(pid);

    uint32_t paddr;
    uint32_t frame_start;
    // even
    vpn = get_vpn(base);
    fst = get_first_level_index(vpn);
    sec = get_second_level_index(vpn);
    if (proc->secondary_pt[fst] == NULL)
    {
        unmapped_page_addr = kalloc(); // use for storing the page
        proc->secondary_pt[fst] = unmapped_page_addr;
    }
    else
    {
        unmapped_page_addr = proc->secondary_pt[fst];
    }
    unmapped_frame_addr = kalloc(); // the physical frame of that page
    frame_start = unmapped_frame_addr;
    paddr = get_paddr(frame_start);
    pfn = get_pfn(paddr);
    entry = COMPRESS_ENTRYLO(pfn, 2, 1, 1, 0);
    proc->secondary_pt[fst][sec].vpn = vpn;
    proc->secondary_pt[fst][sec].entry = entry;
    base += PAGE_SIZE;
    // odd
    vpn = get_vpn(base);
    fst = get_first_level_index(vpn);
    sec = get_second_level_index(vpn);
    if (proc->secondary_pt[fst] == NULL)
    {
        unmapped_page_addr = kalloc(); // use for storing the page
        proc->secondary_pt[fst] = unmapped_page_addr;
    }
    else
    {
        unmapped_page_addr = proc->secondary_pt[fst];
    }
    unmapped_frame_addr = kalloc(); // the physical frame of that page
    frame_start = unmapped_frame_addr;
    paddr = get_paddr(frame_start);
    pfn = get_pfn(paddr);
    entry = COMPRESS_ENTRYLO(pfn, 2, 1, 1, 0);
    proc->secondary_pt[fst][sec].vpn = vpn;
    proc->secondary_pt[fst][sec].entry = entry;
    return;
}

// Handle page missing
void page_miss_handler(uint32_t vpn2)
{
    add_2_page(vpn2, current_running->pid);
}

// // Used when TLB exception occurs
// void TLB_exception()
// {
//     // TLB refill or TLB invalid
//     // Read the badvaddr
//     uint32_t badvaddr;
//     uint32_t epc;
//     __asm__ volatile(
//         "mfc0 %0, $8 \t\n" // cp0_badvaddr
//         "mfc0 %1, $14"     // cp0 epc
//         : "=r"(badvaddr), "=r"(epc)
//         :);
//     uint32_t vpn = get_vpn(badvaddr);
//     vpn &= 0xfffffffe;
//     PTE_t *pte1 = querry_PTE(vpn);
//     if (pte1 == NULL)
//     {
//         // No page. Then we need add a page.
//         // Create 2 pages at once
//         add_2_page(vpn >> 1);
//         pte1 = querry_PTE(vpn);
//     }
//     PTE_t *pte2 = querry_PTE(vpn | 0x1);
//     uint32_t hi = ((pte1->vpn >> 1) << 13) | (current_running->pid & 0xff);
//     uint32_t index;
//     // To check if it is TLB invalid
//     __asm__ volatile(
//         "mtc0 %1, $10 \t\n" // cp0_entryhi
//         "tlbp \t\n"
//         "mfc0 %0, $0" // cp0_index
//         : "=r"(index)
//         : "r"(hi));
//     if (index & 0x80000000)
//     {
//         // Not found, which means we could write at a random position.
//         write_TLB_rand(pte1, pte2);
//     }
//     else
//     {
//         index &= 0b111111;
//         write_TLB_index(pte1, pte2, index);
//     }
// }
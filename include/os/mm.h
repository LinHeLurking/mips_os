#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_

#include "type.h"

// page size is set as 4KB, so the mask reg is all 0.

#define PAGE_SIZE (4096)
#define PTE_SIZE (sizeof(struct PTE_t))
#define SECONDARY_PTE_NUM (PAGE_SIZE / PTE_SIZE)

#define COMPRESS_ENTRYLO(pfn, coherency, dirty, valid, global) \
    ((pfn << 6) | (coherency << 3) | (dirty << 2) | (valid << 1) | global)

typedef struct PTE_t
{
    uint32_t vpn;   // virtual page number
    uint32_t entry; // entrylox regsiter content
} PTE_t;

void do_TLB_Refill();
void do_page_fault();

// Iintialize the memory
void do_mem_init();

// Handle page missing
void page_miss_handler(uint32_t);

// Only needed in task 1.
// Initialize some global page table entries.
void do_page_table_static_init();

// Search in current_running's page table
PTE_t *querry_PTE(uint32_t vpn, int pid);

// Write 2 continuous PTEs into TLB with a given index
void write_TLB_index(PTE_t *a, PTE_t *b, uint32_t index);

// Write 2 continuous PTES into a random TLB
void write_TLB_rand(PTE_t *a, PTE_t *b);

// Add 2 continuous virtual pages at once
void add_2_page(uint32_t vpn2, int pid);

// Used when TLB exception occurs
void TLB_exception();

// Allocate a physical page frame, and return the starting unmapped addr.
void *kalloc();

// Free a physical page fram
void kfree(void *unmapped_vaddr);

// helper functions

// Get the virtual page number 2 of a vaddr
uint32_t get_vpn2(uint32_t vaddr);

// Get the virtual page number of a vaddr
uint32_t get_vpn(uint32_t vaddr);

// Get the physical frame number of a paddr
uint32_t get_pfn(uint32_t paddr);

// Get the start unmapped addr of a frame
void *get_frame_start(void *unmapped);

// Get paddr from an unmapped addr
void *get_paddr(void *unmapped);

// Get the first level page table index from vpn
uint32_t get_first_level_index(uint32_t vpn);

// Get the second level page table index from vpn
uint32_t get_second_level_index(uint32_t vpn);

#endif

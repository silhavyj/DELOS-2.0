#ifndef _PAGING_H_
#define _PAGING_H_

#include <stdint.h>

// https://wiki.osdev.org/Paging

#define PAGE_TABLE_COUNT        1024     // page dir has 1024 entries (page tables)
#define PAGE_TABLE_ENTRIES      1024     // each page table has 1024 entries (pages)

#define PAGE_DIR_ADDR           0x9A000  // addr of the page dir as defined in loader.asm
#define PAGE_TABLE_0_ADDR       0x9B000  // addr of the page table 0 (bios, vide mem, etc)
#define PAGE_TABLE_1_ADDR       0x9C000  // addr of the page table 1 (for all the other page tables)
#define PAGE_TABLE_768_ADDR     0x9D000  // addr of the page table 768 (kernel 4MB)
#define PAGE_TABLE_START_ADDR   0x400000 // addr where we start allocation all the other page tables

#define FRAME_SIZE              0x1000                                       // 4KB
#define ADDRESS_SPACE_SIZE      (4LL * 1024 * 1024 * 1024)                   // 4GB
#define FRAMES_COUNT            (1 + (ADDRESS_SPACE_SIZE / FRAME_SIZE / 32)) // the size of a bitmap if we had all 4GB or RAM

#define KERNEL_HEAP_SIZE        (20 * 1024 * 1024) // kernel heap is 20MB in size (should be table-aligned -> x * 4MB)
#define KERNEL_HEAP_START_PAGE  769                // the very next page after the kernel page
#define KERNEL_HEAP_END_PAGE    (KERNEL_HEAP_START_PAGE + KERNEL_HEAP_SIZE / (PAGE_TABLE_ENTRIES * FRAME_SIZE) - 1)
#define KERNEL_HEAP_START_ADDR  (0xC0400000)       // 0xC0000000 + 4MB

#define FS_START_ADDR           (KERNEL_HEAP_START_ADDR + KERNEL_HEAP_SIZE)
#define FS_SIZE                 (20 * 1024 * 1024)         // file system size (20 MB, should be table-aligned -> x * 4MB)
#define FS_START_PAGE           (KERNEL_HEAP_END_PAGE + 1) // the very next page after the kernel heap
#define FS_END_PAGE             (FS_START_PAGE + FS_SIZE / (PAGE_TABLE_ENTRIES * FRAME_SIZE) - 1)

#define PAGE_TABLE_ADDR(pt_index) ((uint32_t)(pt_index) * PAGE_TABLE_ENTRIES * FRAME_SIZE)
#define TOP_STACK_ADDR(pt_index) ((((uint32_t)(pt_index) + 1) * PAGE_TABLE_ENTRIES * FRAME_SIZE) - 1)

typedef struct {
    uint32_t present            : 1;    // bit 0
    uint32_t read_write         : 1;    // bit 1
    uint32_t user_mode          : 1;    // bit 2
    uint32_t write_through      : 1;    // bit 3
    uint32_t cache_disabled     : 1;    // bit 4
    uint32_t accessed           : 1;    // bit 5
    uint32_t dirty              : 1;    // bit 6
    uint32_t zero               : 1;    // bit 7
    uint32_t global             : 1;    // bit 8
    uint32_t available          : 3;    // bits [9-11]
    uint32_t physical_page_addr : 20;   // bits [12-31]
} __attribute__((packed)) page_table_entry_t;

typedef struct {
    page_table_entry_t pages[PAGE_TABLE_ENTRIES];
} __attribute__((packed)) page_table_t;

typedef struct {
    uint32_t present            : 1;    // bit 0
    uint32_t read_write         : 1;    // bit 1
    uint32_t user_mode          : 1;    // bit 2
    uint32_t write_through      : 1;    // bit 3
    uint32_t cache_disabled     : 1;    // bit 4
    uint32_t accessed           : 1;    // bit 5
    uint32_t zero               : 1;    // bit 6
    uint32_t page_size          : 1;    // bit 7
    uint32_t ignored            : 1;    // bit 8
    uint32_t available          : 3;    // bits [9-11]
    uint32_t page_table_addr    : 20;   // bits [12-31]
} __attribute__((packed)) page_directory_entry_t;

typedef struct {
    page_directory_entry_t page_tables[PAGE_TABLE_ENTRIES];
} __attribute__((packed)) page_dir_t;

int paging_init();
uint32_t allocate_page_table(uint32_t page_table_index, uint8_t user);
uint32_t allocate_page(uint32_t page_table_index, uint32_t page_index, uint8_t user);
uint32_t allocate_frame();
void frame_set_state(uint32_t frame_index, uint32_t occupied);
uint32_t allocate_page(uint32_t user);
void unmap_page(uint32_t virtual_addr);
uint32_t get_number_of_free_frames();

#endif
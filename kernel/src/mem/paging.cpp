#include <mem/paging.h>
#include <common.h>
#include <memory.h>
#include <drivers/screen/screen.h>

// restore the page directory from the memory
page_dir_t *kernel_page_dir = reinterpret_cast<page_dir_t *>(PAGE_DIR_ADDR);

// bitmap to keep track of which frames are free
static uint32_t frames[FRAMES_COUNT];

// the actual number of physical frames (physical size e.g. 512 MB reduced by the kernel size)
static uint32_t physical_frames;
static uint32_t frames_bitmap_size;

extern uint32_t kernel_size;       // the size of the kernel
extern uint32_t physical_mem_size; // how much RAM we gave to the VM

uint32_t last_page_table_addr;

static void map_kernel_heap();
static void map_filesystem();

uint32_t get_number_of_free_frames() {
    uint32_t free_frames = 0;
    uint32_t i, j;
    for (i = 0; i < frames_bitmap_size; i++) {
        if (frames[i] != 0xFFFFFFFF)
            for (j = 0; j < 32; j++)
                free_frames += !((frames[i] >> j) & 1);
    }
    return free_frames;
}

int paging_init() {
    // clear out the bitmap and calculate the actual number of frames
    // based on the physical mem - the size of the kernel
    // (+1 so we're page-aligned)
    memset(frames, 0, FRAMES_COUNT * sizeof(uint32_t));

    // calculate the number of physical frames
    physical_frames = physical_mem_size / FRAME_SIZE - (kernel_size / FRAME_SIZE);
    if (kernel_size % FRAME_SIZE != 0)
        physical_frames--;

    // calculate the size of the bitmap based on the number of frames
    frames_bitmap_size = physical_frames / 32;
    if (physical_frames % 32 != 0) {
        frames_bitmap_size++;
        // lock the bits off in the last slot (it's not fully occupied - remainder)
        frames[frames_bitmap_size - 1] |= ~(1 << (31 - physical_frames % 32));
    }

    // lock the first 4MB so we cannot accidentally
    // map something onto there again
    uint32_t i;
    for (i = 0; i < 2 * (0x400000 / (FRAME_SIZE * 32)); i++) {
        frames[i] = 0xFFFFFFFF;
    }

    // clear out all page tables (each entry of the page directory
    // except for the page table 0 and page table 768 - those have
    // been mapped in loader.asm)
    for (i = 0; i < PAGE_TABLE_COUNT; i++) {
        if (i == 0 || i == 1 || i == 768)
            continue;
        memset(&kernel_page_dir->page_tables[i], 0, sizeof(page_directory_entry_t));
        kernel_page_dir->page_tables[i].page_table_addr = 0xFFFFF;
    }

    // start putting the page tables into the second MB
    last_page_table_addr = PAGE_TABLE_START_ADDR;

    // reserve pages for the kernel heap
    map_kernel_heap();

    // reserve pages for the filesystem
    map_filesystem();

    return 0;
}

static void map_filesystem() {
    uint32_t i, j;
    for (i = FS_START_PAGE; i <= FS_END_PAGE; i++) {
        allocate_page_table(i, 1);
        for (j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            allocate_page(i, j, 1);
        }
    }
}

static void map_kernel_heap() {
    // go through the appropriate page tables
    // and in each step - allocate a new page table and also,
    // allocate all its pages as well
    uint32_t i, j;
    for (i = KERNEL_HEAP_START_PAGE; i <= KERNEL_HEAP_END_PAGE; i++) {
        allocate_page_table(i, 1);
        for (j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            allocate_page(i, j, 1);
        }
    }
}

uint32_t allocate_frame() {
    uint32_t i, j;
    uint32_t frame_number;

    // iterate through the bitmap until
    // you find a free frame. Once you find it
    // set it as 'used' and return its index
    for (i = 0; i < frames_bitmap_size; i++) {
        if (frames[i] != 0xFFFFFFFF) {    // all frames occupied
            for (j = 0; j < 32; j++)
                if (((frames[i] >> j) & 1) == 0) {
                    frame_number = i * 32 + j;
                    frame_set_state(frame_number, 1);
                    return frame_number;
                }
        }
    }
    // if no free frame has been found, just panic the system for now
    set_color(FOREGROUND_RED);
    kprintf("ERR: all frames are taken up!");
    _panic();
    return 0;
}

void frame_set_state(uint32_t frame_index, uint32_t occupied) {
    // set the state of a frame (flip the appropriate bit
    // according to its new state - occupied/occupied)
    uint32_t byte_num = frame_index / 32;
    uint32_t bit_num  = frame_index % 32;
    uint32_t mask     = 1 << bit_num;

    if (occupied == 1)
        frames[byte_num] |= mask;
    else if (occupied == 0)
        frames[byte_num] &= ~mask;
}

uint32_t allocate_page_table(uint32_t page_table_index, uint8_t user) {
    // make sure the page table has indeed not been allocated yet
    if (kernel_page_dir->page_tables[page_table_index].page_table_addr != 0xFFFFF)
        return kernel_page_dir->page_tables[page_table_index].page_table_addr << 12;

    page_table_t *page_table = (page_table_t *)last_page_table_addr;

    // init the pages in the page table clear all bits out and
    // set the physical address to the maximum (default unused state)
    uint32_t i;
    for (i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        memset(&page_table->pages[i], 0, sizeof(page_table_entry_t));
        page_table->pages[i].physical_page_addr = 0xFFFFF;
    }

    // let the page dir know of the presence of the page table and where it's located
    kernel_page_dir->page_tables[page_table_index].page_table_addr = (last_page_table_addr & 0xFFFFF000) >> 12;
    kernel_page_dir->page_tables[page_table_index].read_write = 1;
    kernel_page_dir->page_tables[page_table_index].present = 1;
    kernel_page_dir->page_tables[page_table_index].user_mode = user;

    // we must also flush the TLB or we might not notice the change
    // https://wiki.osdev.org/Paging#Virtual_Memory
    _flush_tlb((uint32_t)(&kernel_page_dir->page_tables[page_table_index]));

    // move on to the next page table address (+ 4KB)
    last_page_table_addr += 0x1000;

    // return the physical addr of the page table
    return (last_page_table_addr - 0x1000);
}

uint32_t allocate_page(uint32_t page_table_index, uint32_t page_index, uint8_t user) {
    // allocate a free frame and calculate the address
    // of the frame based on the frame index
    uint32_t frame_index = allocate_frame();
    uint32_t frame_addr = frame_index * FRAME_SIZE;

    // make sure the page table has been allocated
    if (kernel_page_dir->page_tables[page_table_index].page_table_addr == 0xFFFFF)
        allocate_page_table(page_table_index, user);

    // load the page table from the memory using the page table directory
    // (it holds the page table address as we set it up in allocate_page_table())
    page_table_t *page_table = (page_table_t *)(kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);

    // make sure the page has not been allocated yet
    if (page_table->pages[page_index].physical_page_addr != 0xFFFFF) {
        return ((page_table_index & 0x3FF) << 22) | ((page_index & 0x3FF) << 12);
    }

    // map the appropriate page onto the frame we just allocated
    page_table->pages[page_index].physical_page_addr = (frame_addr & 0xFFFFF000) >> 12;
    page_table->pages[page_index].read_write = 1;
    page_table->pages[page_index].present = 1;
    page_table->pages[page_index].user_mode = user;

    // we should also flush the TLB so the change takes place
    _flush_tlb(frame_addr);

    // return the virtual address of the page
    return ((page_table_index & 0x3FF) << 22) | ((page_index & 0x3FF) << 12);
}

uint32_t allocate_page(uint32_t user) {
    uint32_t i, j;
    page_table_t *page_table;

    // iterate through the page directory and look for
    // a page table which has already been mapped
    for (i = 0; i < PAGE_TABLE_COUNT; i++) {
        // skip the very first two pages as well as the kernel page
        if (i == 0 || i == 1 || i >= 768)
            continue;
        // if such a table exists, continue iterating through all
        // its pages and look for a page which has not been mapped yet
        if (kernel_page_dir->page_tables[i].page_table_addr != 0xFFFFF) {
            page_table = (page_table_t *)(kernel_page_dir->page_tables[i].page_table_addr << 12);

            // if you find an unallocated page, map it and return the
            // corresponding virtual address
            for (j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                if (page_table->pages[j].physical_page_addr == 0xFFFFF) {
                    return allocate_page(i, j, user);
                }
            }
        } else {
            // if the current page table has not been mapped yet,
            // map it and also map the very first page in that table
            allocate_page_table(i, user);
            return allocate_page(i, 0, user);
        }
    }
    set_color(FOREGROUND_RED);
    kprintf("ERR: all pages are already mapped!");
    _panic();
    return 0;
}

void unmap_page(uint32_t virtual_addr) {
    // calculate the page table index and page index
    // from the virtual address
    uint32_t page_table_index = virtual_addr >> 22;
    uint32_t page_index = (virtual_addr >> 12) & 0x3FF;

    // load the corresponding page table from the memory
    page_table_t *page_table = (page_table_t *) (kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);

    // make sure the page has indeed been mapped
    if (page_table->pages[page_index].physical_page_addr == 0xFFFFF)
        return;

    // set the frame which the page was mapped to as free
    uint32_t frame_addr = page_table->pages[page_index].physical_page_addr << 12;
    uint32_t frame_index = frame_addr / FRAME_SIZE;
    frame_set_state(frame_index, 0);

    // clear out the page entry and set the address to the default (unused) value
    memset(&page_table->pages[page_index], 0, sizeof(page_table_entry_t));
    page_table->pages[page_index].physical_page_addr = 0xFFFFF;

    // we should also flush the TLB so the change takes place
    _flush_tlb(virtual_addr);
}
#include <memory.h>
#include <string.h>
#include <common.h>
#include <limits.h>
#include <drivers/screen/screen.h>
#include <processes/process.h>
#include <processes/user_programs.h>
#include <processes/elf_loader.h>

static uint8_t pids[MAX_NUMBER_OF_PROCESSES];
static uint32_t process_count;

static void print_pcb_state(uint8_t pcb_state);

static page_dir_t *allocate_page_dir(page_dir_t **process_page_dir_virt);
static uint32_t allocate_stack_page(page_dir_t *process_page_dir, PCB_t *pcb);
static uint32_t allocate_heap_pages(page_dir_t *process_page_dir, PCB_t *pcb);

extern page_dir_t *kernel_page_dir;

static void print_pcb_state(uint8_t pcb_state) {
    switch (pcb_state) {
        case PROCESS_STATE_NEW:
            kprintf("NEW\n\r");
            break;
        case PROCESS_STATE_RUNNING:
            kprintf("RUNNING\n\r");
            break;
        case PROCESS_STATE_WAITING:
            kprintf("WAITING\n\r");
            break;
        case PROCESS_STATE_READY:
            kprintf("READY\n\r");
            break;
        case PROCESS_STATE_TERMINATION:
            kprintf("TERMINATION\n\r");
            break;
    }
}

void print_pcb(void *data) {
    PCB_t *pcb = (PCB_t *)data;
    set_color(FOREGROUND_LIGHTGRAY);
    kprintf("PID=");
    reset_color();
    kprintf("%d", pcb->pid);

    set_color(FOREGROUND_LIGHTGRAY);
    kprintf(" | PPID=");
    reset_color();
    kprintf("%d", pcb->ppid);

    set_color(FOREGROUND_LIGHTGRAY);
    kprintf(" | NAME=");
    reset_color();
    kprintf("%s", pcb->name);

    set_color(FOREGROUND_LIGHTGRAY);
    kprintf(" | STATE=");
    reset_color();
    print_pcb_state(pcb->state);
}

void print_registers(PCB_t *pcb) {
    kprintf("eax = 0x%x\n\r", pcb->regs.eax);
    kprintf("ecx = 0x%x\n\r", pcb->regs.ecx);
    kprintf("edx = 0x%x\n\r", pcb->regs.edx);
    kprintf("ebx = 0x%x\n\r", pcb->regs.ebx);
    kprintf("esp = 0x%x\n\r", pcb->regs.esp);
    kprintf("ebp = 0x%x\n\r", pcb->regs.ebp);
    kprintf("esi = 0x%x\n\r", pcb->regs.esi);
    kprintf("edi = 0x%x\n\r", pcb->regs.edi);
    kprintf("eflags = 0x%x\n\r", pcb->regs.eflags);
    kprintf("cr3 = 0x%x\n\r", pcb->regs.cr3);
    kprintf("eip = 0x%x\n\r", pcb->regs.eip);
}

PCB_t *create_process_virtual_addr_space(const char *filename, uint32_t ppid, const char *stdout, uint32_t shell_id) {
    // make sure there's currently running a reasonable amount of processes
    if (process_count >= MAX_NUMBER_OF_PROCESSES)
        return NULL;

    // get the program we want to instantiate (create a process off of it)
    program_t *program = get_program(filename);
    if (program == NULL)
        return NULL;

    // create a new pcb
    PCB_t *pcb = (PCB_t *)kmalloc(sizeof(PCB_t));
    pcb->pid = allocate_pid();
    pcb->ppid = ppid;
    pcb->state = PROCESS_STATE_NEW;
    pcb->shell_id = shell_id;

    strcpy(pcb->name, filename);
    strcpy(pcb->stdout, stdout);

    pcb->open_files = list_create();
    pcb->page_tables = list_create();

    // clear out all registers
    memset(&pcb->regs, 0, sizeof(regs_t));

    // initialize flags, cr3, esp, and eip
    pcb->regs.eflags = EFLAGS_ALWAYS1_BIT | EFLAGS_ENABLE_IF_BIT;
    pcb->regs.cr3 = (uint32_t)allocate_page_dir(&pcb->page_dir_kernel_mapping);
    pcb->regs.esp = allocate_stack_page(pcb->page_dir_kernel_mapping, pcb);
    load_elf_file(filename, pcb);

    // we have to temporarily switch over to the address space of the process, so we
    // can initialize its heap (we cannot initialize it from the current address space - it's not mapped here)
    uint32_t heap_start_addr = allocate_heap_pages(pcb->page_dir_kernel_mapping, pcb);
    _load_page_dir(pcb->regs.cr3);
    heap_init(&pcb->heap, heap_start_addr, PROCESS_HEAP_SIZE);
    _load_page_dir(PAGE_DIR_ADDR);

    return pcb;
}

void free_pid(uint32_t pid) {
    pids[pid] = 0;
}

uint32_t allocate_pid() {
    uint32_t i;
    for (i = 0; i < MAX_NUMBER_OF_PROCESSES; i++)
        if (pids[i] == 0) {
            pids[i] = 1;
            return i;
        }
    return UINT_MAX; // it should never get here
}

int init_processes() {
    memset(&pids, 0, sizeof(pids));
    process_count = 0;
    return 0;
}

static page_dir_t *allocate_page_dir(page_dir_t **process_page_dir_virt) {
    // allocate a free page where we can create a new page directory
    // we can set it up using virtual address as it will be converted
    // into physical addresses by the CPU anyways
    uint32_t page_virtual_addr = allocate_page(1);
    page_dir_t *process_page_dir = (page_dir_t *)(page_virtual_addr);

    // calculate the page table index as well as the page index of that virtual address
    // the process needs to know the physical address of the page, so it can store it into its CR3 register
    uint32_t page_table_index = page_virtual_addr >> 22;
    uint32_t page_index = (page_virtual_addr >> 12) & 0x3FF;
    page_table_t *page_table = (page_table_t *)(kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);
    uint32_t process_page_dir_physical_addr = page_table->pages[page_index].physical_page_addr << 12;

    // copy everything that is mapped into the kernel's address space to the new address space as well
    // (kernel higher-half, kernel heap, virtual file system, ...)
    // the process won't be able to access them as they have been assigned different access rights

    // clear out all page tables in the new address space
    uint32_t i;
    for (i = 1; i < PAGE_TABLE_COUNT; i++) {
        memset(&process_page_dir->page_tables[i], 0, sizeof(page_directory_entry_t));
        process_page_dir->page_tables[i].page_table_addr = 0xFFFFF;
    }

    // map the first page table (BIOS, screen, ...) into the new address space
    process_page_dir->page_tables[0].page_table_addr = (PAGE_TABLE_0_ADDR & 0xFFFFF000)>> 12;
    process_page_dir->page_tables[0].present = 1;
    process_page_dir->page_tables[0].read_write = 1;
    process_page_dir->page_tables[0].user_mode = 0;

    process_page_dir->page_tables[1].page_table_addr = (PAGE_TABLE_1_ADDR & 0xFFFFF000)>> 12;
    process_page_dir->page_tables[1].present = 1;
    process_page_dir->page_tables[1].read_write = 1;
    process_page_dir->page_tables[1].user_mode = 0;

    // map the kernel into higher-half of the new address space
    process_page_dir->page_tables[768].page_table_addr = (PAGE_TABLE_768_ADDR & 0xFFFFF000)>> 12;
    process_page_dir->page_tables[768].present = 1;
    process_page_dir->page_tables[768].read_write = 1;
    process_page_dir->page_tables[768].user_mode = 0;

    // map the kernel heap into the new address space
    for (i = KERNEL_HEAP_START_PAGE; i <= KERNEL_HEAP_END_PAGE; i++) {
        memcpy(&process_page_dir->page_tables[i], &kernel_page_dir->page_tables[i], sizeof(page_table_entry_t));
    }

    // map the file system into the new address space
    for (i = FS_START_PAGE; i <= FS_END_PAGE; i++) {
        memcpy(&process_page_dir->page_tables[i], &kernel_page_dir->page_tables[i], sizeof(page_table_entry_t));
    }

    // store the virtual address of the process page dir (mapped in the kernel address space)
    // we'll need this later on e.g. when allocating user stack
    // if we used the physical address it'd result in a page fault as the CR3 register has
    // not been changed to the new address space
    *process_page_dir_virt = process_page_dir;

    // return the physical address of the new page directory so the process
    // can store it into its CR3 register
    return (page_dir_t *)process_page_dir_physical_addr;
}

static uint32_t allocate_stack_page(page_dir_t *process_page_dir, PCB_t *pcb) {
    // allocate an empty page that we can use as a stack page table
    // we can initialize it using the kernel page dir
    uint32_t page_virtual_addr = allocate_page(1);
    page_table_t *stack_page_table = (page_table_t *)(page_virtual_addr);

    list_add_last(pcb->page_tables, stack_page_table);

    // but we'll have to map this page as a page table in the process's page directory, hence we need to get the physical addr as well
    uint32_t page_table_index = page_virtual_addr >> 22;
    uint32_t page_index = (page_virtual_addr >> 12) & 0x3FF;
    page_table_t *page_table = (page_table_t *)(kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);
    uint32_t stack_page_table_physical_addr = page_table->pages[page_index].physical_page_addr << 12;

    uint32_t i;
    uint32_t physical_addr;

    // allocate 1024 pages and store them into the stack page table
    // so the stack will be expanded up to 4MB (it uses the whole page table)
    for (i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        // get a free frame
        physical_addr = allocate_frame() * FRAME_SIZE;

        // map the page into the stack page table
        stack_page_table->pages[i].physical_page_addr = (physical_addr & 0xFFFFF000) >> 12;
        stack_page_table->pages[i].read_write = 1;
        stack_page_table->pages[i].user_mode = 1;
        stack_page_table->pages[i].present = 1;
    }

    // map the stack page table itself into the process's page directory - 767 is the page right below kernel
    process_page_dir->page_tables[PROCESS_STACK_PAGE_TABLE].page_table_addr = (stack_page_table_physical_addr & 0xFFFFF000) >> 12;
    process_page_dir->page_tables[PROCESS_STACK_PAGE_TABLE].read_write = 1;
    process_page_dir->page_tables[PROCESS_STACK_PAGE_TABLE].user_mode = 1;
    process_page_dir->page_tables[PROCESS_STACK_PAGE_TABLE].present = 1;

    // return virtual address of the ESP register
    // the user page directory needs to be loader into CR3 beforehand so this address will get translated correctly
    return TOP_STACK_ADDR(PROCESS_STACK_PAGE_TABLE);
}

static uint32_t allocate_heap_pages(page_dir_t *process_page_dir, PCB_t *pcb) {
    uint32_t i, j;
    uint32_t page_virtual_addr;
    page_table_t *heap_page_table;

    uint32_t physical_addr;
    uint32_t page_table_index;
    uint32_t page_index;
    page_table_t *page_table;
    uint32_t heap_page_table_physical_addr;

    // go over the page tables that will make up heap
    for (i = PROCESS_HEAP_START_PAGE_TABLE; i <= PROCESS_HEAP_END_PAGE_TABLE; i++) {
        // get a free page and treat is as a page table
        page_virtual_addr = allocate_page(1);
        heap_page_table = (page_table_t *)(page_virtual_addr);

        list_add_last(pcb->page_tables, heap_page_table);

        // get the physical address of the page table, so we can map it into process_page_dir
        page_table_index = page_virtual_addr >> 22;
        page_index = (page_virtual_addr >> 12) & 0x3FF;
        page_table = (page_table_t *)(kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);
        heap_page_table_physical_addr = page_table->pages[page_index].physical_page_addr << 12;

        // let process_page_dir know of the page table we just allocated
        process_page_dir->page_tables[i].page_table_addr = (heap_page_table_physical_addr & 0xFFFFF000) >> 12;
        process_page_dir->page_tables[i].read_write = 1;
        process_page_dir->page_tables[i].user_mode = 1;
        process_page_dir->page_tables[i].present = 1;

        // allocate all pages within the page table
        for (j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            // get a free page frame
            physical_addr = allocate_frame() * FRAME_SIZE;

            // let the page table know of the page we just allocated
            heap_page_table->pages[j].physical_page_addr = (physical_addr & 0xFFFFF000) >> 12;
            heap_page_table->pages[j].read_write = 1;
            heap_page_table->pages[j].user_mode = 1;
            heap_page_table->pages[j].present = 1;
        }
    }
    // return the start address of the user heap
    return PAGE_TABLE_ADDR(PROCESS_HEAP_START_PAGE_TABLE);
}

static void delete_page_table_record(void *data) {
    kfree(data);
}

void unmap_process(PCB_t *pcb) {
    uint32_t i;
    page_table_t *page_table;
    uint32_t frame_addr;
    uint32_t frame_index;

    // make sure we have the kernel mapping
    _load_page_dir(PAGE_DIR_ADDR);

    while (pcb->page_tables->size != 0) {
        page_table = (page_table_t *)list_get(pcb->page_tables, 0);
        list_remove(pcb->page_tables, 0, delete_page_table_record);

        for (i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            if (page_table->pages[i].physical_page_addr != 0xFFFFF) {
                frame_addr = page_table->pages[i].physical_page_addr << 12;
                frame_index = frame_addr / FRAME_SIZE;
                frame_set_state(frame_index, 0);
            }
        }
    }
    unmap_page((uint32_t)pcb->page_dir_kernel_mapping);
    list_free(&pcb->page_tables, NULL);
}

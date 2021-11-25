#include <processes/elf_loader.h>
#include <processes/user_programs.h>
#include <processes/list.h>
#include <mem/paging.h>
#include <memory.h>
#include <common.h>

// #define ELF_DEBUG

#ifdef ELF_DEBUG
# include <drivers/screen/screen.h>
#endif

extern page_dir_t *kernel_page_dir;

static int validate_elf(elf_header_t *header) {
    return !(header->e_ident[0] == 0x7f && header->e_ident[1] == 'E' && header->e_ident[2] == 'L' && header->e_ident[3] == 'F');
}

static void free_index(void *data) {
    kfree(data);
}

static uint8_t page_table_indexes_cmp(void *data1, void *data2) {
    return *(uint32_t *)data1 == *(uint32_t *)data2;
}

static void allocate_segment(uint32_t start_virt_addr, uint32_t end_virt_addr, list_t *page_table_indexes, PCB_t *pcb, char *data, uint32_t data_size) {
    uint32_t page_table_start = start_virt_addr >> 22;
    uint32_t page_start = (start_virt_addr >> 12) & 0x3FF;

    uint32_t page_table_end = end_virt_addr >> 22;
    uint32_t page_end = (end_virt_addr >> 12) & 0x3FF;

    #ifdef ELF_DEBUG
        kprintf("TABLE_START=%d PAGE_START=%d TABLE_END=%d PAGE_END=%d\n\r", page_table_start, page_start, page_table_end, page_end);
    #endif

    uint32_t i, j;
    uint32_t *page_table_index_storage;
    uint32_t page_table_virt_addr;
    uint32_t page_table_phys_addr;
    page_table_t *code_page_table;
    page_table_t *page_table;
    uint32_t page_table_index;
    uint32_t page_index;
    uint32_t page_virt_addr;
    uint32_t page_phys_addr;

    list_t *data_pages = list_create();
    uint32_t *page_virt_addr_storage;

    for (i = page_table_start; i <= page_table_end; i++) {
        page_table_index_storage = (uint32_t *) kmalloc(sizeof(uint32_t));
        *page_table_index_storage = i;

        if (list_contains(page_table_indexes, page_table_index_storage, page_table_indexes_cmp) == 1) {
            code_page_table = (page_table_t *) list_get(pcb->page_tables, pcb->page_tables->size - 1);
            kfree(page_table_index_storage);
        } else {
            page_table_virt_addr = allocate_page(1);
            code_page_table = (page_table_t *) page_table_virt_addr;

            list_add_last(pcb->page_tables, code_page_table);

            for (j = 0; j < PAGE_TABLE_ENTRIES; j++) {
                memset(&code_page_table->pages[j], 0, sizeof(page_table_entry_t));
                code_page_table->pages[j].physical_page_addr = 0xFFFFF;
            }

            list_add_last(page_table_indexes, page_table_index_storage);

            page_table_index = page_table_virt_addr >> 22;
            page_index = (page_table_virt_addr >> 12) & 0x3FF;
            page_table = (page_table_t *) (kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);
            page_table_phys_addr = page_table->pages[page_index].physical_page_addr << 12;

            pcb->page_dir_kernel_mapping->page_tables[i].page_table_addr = (page_table_phys_addr & 0xFFFFF000) >> 12;
            pcb->page_dir_kernel_mapping->page_tables[i].read_write = 1;
            pcb->page_dir_kernel_mapping->page_tables[i].user_mode = 1;
            pcb->page_dir_kernel_mapping->page_tables[i].present = 1;
        }

        for (j = (i == page_table_start ? page_start : 0); j <= (i == page_table_end ? page_end : (PAGE_TABLE_ENTRIES - 1)); j++) {
            page_virt_addr = allocate_page(1);

            page_virt_addr_storage = (uint32_t *)kmalloc(sizeof(uint32_t));
            *page_virt_addr_storage = page_virt_addr;
            list_add_last(data_pages, page_virt_addr_storage);

            page_table_index = page_virt_addr >> 22;
            page_index = (page_virt_addr >> 12) & 0x3FF;
            page_table = (page_table_t *) (kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);
            page_phys_addr = page_table->pages[page_index].physical_page_addr << 12;

            code_page_table->pages[j].physical_page_addr = (page_phys_addr & 0xFFFFF000) >> 12;
            code_page_table->pages[j].read_write = 1;
            code_page_table->pages[j].user_mode = 1;
            code_page_table->pages[j].present = 1;
        }
    }
    uint32_t pages_needed = data_size / FRAME_SIZE;
    uint32_t remainder = data_size % FRAME_SIZE;
    if (remainder != 0)
        pages_needed++;

    uint32_t offset = 0;

    _load_page_dir(PAGE_DIR_ADDR);

    for (i = 0; i < pages_needed - 1; i++) {
        page_virt_addr = *(uint32_t *)list_get(data_pages, 0);
        list_remove(data_pages, 0, free_index);
        memcpy((char *)page_virt_addr, &data[offset], FRAME_SIZE);
        offset += FRAME_SIZE;

        // unmap the page from the kernel's virtual address space
        // the frame remains occupied so the process can use it within its own address space
        page_table_index = page_virt_addr >> 22;
        page_index = (page_virt_addr >> 12) & 0x3FF;
        page_table = (page_table_t *)(kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);
        memset(&page_table->pages[page_index], 0, sizeof(page_table_entry_t));
        page_table->pages[page_index].physical_page_addr = 0xFFFFF;
    }

    page_virt_addr = *(uint32_t *)list_get(data_pages, 0);
    list_remove(data_pages, 0, free_index);
    memcpy((char *)page_virt_addr, &data[offset], remainder);

    // unmap the page from the kernel's virtual address space
    // the frame remains occupied so the process can use it within its own address space
    _load_page_dir(PAGE_DIR_ADDR);
    page_table_index = page_virt_addr >> 22;
    page_index = (page_virt_addr >> 12) & 0x3FF;
    page_table = (page_table_t *)(kernel_page_dir->page_tables[page_table_index].page_table_addr << 12);
    memset(&page_table->pages[page_index], 0, sizeof(page_table_entry_t));
    page_table->pages[page_index].physical_page_addr = 0xFFFFF;

    list_free(&data_pages, NULL);
}

int load_elf_file(const char *filename, PCB_t *pcb) {
    program_t *program = get_program(filename);
    if (program == NULL)
        return 1;

    elf_header_t *header = (elf_header_t *)program->code;
    if (validate_elf(header) == 1) {
        return 1;
    }

    #ifdef ELF_DEBUG
        kprintf("Type: %s%s%s\n\r",
               header->e_ident[4] == 1 ? "32bit ":"64 bit",
               header->e_ident[5] == 1 ? "Little Endian ":"Big endian ",
               header->e_ident[6] == 1 ? "True ELF ":"buggy ELF ");
    #endif

    uint32_t i;
    list_t *page_table_indexes = list_create();
    elf_program_header_t *ph = (elf_program_header_t *)(program->code + header->e_phoff);

    for(i = 0; i < header->e_phnum; i++, ph++) {
        if (ph->p_type == 1) {
            #ifdef ELF_DEBUG
                kprintf("LOAD: offset 0x%x vaddr 0x%x paddr 0x%x filesz 0x%x memsz 0x%x\n\r", ph->p_offset, ph->p_vaddr, ph->p_paddr, ph->p_filesz, ph->p_memsz);
            #endif
            allocate_segment(ph->p_vaddr, ph->p_vaddr + ph->p_filesz, page_table_indexes, pcb, (char *)((uint32_t)program->code + ph->p_offset), ph->p_filesz);
            if(ph->p_flags == PF_X + PF_R + PF_W || ph->p_flags == PF_X + PF_R) {
                pcb->regs.eip = header->e_entry;
                #ifdef ELF_DEBUG
                    kprintf("EIP = 0x%x\n\r", pcb->regs.eip);
                #endif
            }
        }
    }
    list_free(&page_table_indexes, free_index);
    return 0;
}
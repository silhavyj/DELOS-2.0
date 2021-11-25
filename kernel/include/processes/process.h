#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <stdint.h>
#include <mem/heap.h>
#include <mem/paging.h>
#include <processes/list.h>

#define PROCESS_STATE_NEW           1
#define PROCESS_STATE_RUNNING       2
#define PROCESS_STATE_WAITING       3
#define PROCESS_STATE_READY         4
#define PROCESS_STATE_TERMINATION   5

//https://en.wikipedia.org/wiki/FLAGS_register

#define EFLAGS_ALWAYS1_BIT           (1 << 1)
#define EFLAGS_ENABLE_IF_BIT         (1 << 9)

#define MAX_NUMBER_OF_PROCESSES       256
#define PROCESS_STACK_PAGE_TABLE      767

#define PROCESS_HEAP_SIZE_IN_4M       2 // 8 * 4MB = 8MB
#define PROCESS_HEAP_SIZE             (PROCESS_HEAP_SIZE_IN_4M * 1024 * 4096)
#define PROCESS_HEAP_START_PAGE_TABLE (PROCESS_STACK_PAGE_TABLE - PROCESS_HEAP_SIZE_IN_4M - 1)
#define PROCESS_HEAP_END_PAGE_TABLE   (PROCESS_HEAP_START_PAGE_TABLE + PROCESS_HEAP_SIZE_IN_4M)

#define PROCESS_NAME_LEN   16
#define PROCESS_STDOUT_LEN 16

typedef struct {
    uint32_t eax;    // 0
    uint32_t ecx;    // 4
    uint32_t edx;    // 8
    uint32_t ebx;    // 12
    uint32_t esp;    // 16
    uint32_t ebp;    // 20
    uint32_t esi;    // 24
    uint32_t edi;    // 28
    uint32_t eflags; // 32
    uint32_t cr3;    // 36
    uint32_t eip;    // 40
} __attribute__((packed)) regs_t;

typedef struct {
    uint32_t pid;
    uint32_t ppid;
    regs_t regs;
    char name[PROCESS_NAME_LEN];
    char stdout[PROCESS_STDOUT_LEN];
    heap_t heap;
    uint8_t state;
    uint32_t shell_id;
    page_dir_t *page_dir_kernel_mapping;
    list_t *open_files;
    list_t *page_tables;
} PCB_t;

int init_processes();
PCB_t *create_process_virtual_addr_space(const char *filename, uint32_t ppid, const char *stdout, uint32_t shell_id);
void print_registers(PCB_t *pcb);
void print_pcb(void *data);
void unmap_process(PCB_t *pcb);
uint32_t allocate_pid();
void free_pid(uint32_t pid);

#endif
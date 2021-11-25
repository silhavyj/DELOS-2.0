#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <processes/process.h>

void print_to_stream(char *buffer);
void print_to_stream(char *buffer, uint8_t addNewLine);
void print_to_stream(PCB_t *pcb, char *buffer);
void print_to_stream(PCB_t *pcb, char *buffer, uint8_t addNewLine);

extern "C" {
    void _panic();
    void _outb(uint16_t addr, uint8_t data);
    uint8_t _inb(uint16_t addr);
    void _load_gdt(uint32_t addr);
    void _io_wait();
    void _load_idt(uint32_t);
    void _enable_interrupts();
    void _disable_interrupts();
    void _enable_paging();
    void _disable_paging();
    void _load_page_dir(uint32_t addr);
    void _flush_tlb(uint32_t addr);
    void _tss_flush(uint32_t addr);
    uint32_t _get_page_dir();
}

#endif
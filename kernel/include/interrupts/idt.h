#ifndef _IDT_H_
#define _IDT_H_

#include <stdint.h>

// https://wiki.osdev.org/Interrupt_Descriptor_Table

#define IDT_GATE_COUNT            256
#define IDT_PRESENT                 1
#define IDT_NOT_PRESENT             0

#define IDT_32_BIT_TASK_GATE      0x5
#define IDT_16_BIT_INTERRUPT_GATE 0x6
#define IDT_16_BIT_TRAP_GATE      0x7
#define IDT_32_BIT_INTERRUPT_GATE 0xE
#define IDT_32_BIT_TRAP_GATE      0xF

// IDT entry - gate
typedef struct {
    uint32_t offset_low         : 16;   // offset bits [0-15] of the interrupt handler
    uint32_t selector           : 16;   // code segment selector in GDT or LDT
    uint32_t zero               :  8;   // unused, set to 0
    uint32_t gate_type          :  4;   // gate type 0..3
    uint32_t S                  :  1;   // Storage Segment; set to 0 for interrupt and trap gates
    uint32_t DPL                :  2;   // Descriptor Privilege Level; minimum privilege level of the calling descriptor
    uint32_t P                  :  1;   // present; set to 0 for unused interrupts
    uint32_t offset_high        : 16;   // offset bits [16-31] of the interrupt handler
} __attribute__((packed)) IDT_gate_t;

// pointer to the interrupt descriptor table
typedef struct {
    uint16_t limit;         // size of the table
    uint32_t base;          // start address
} __attribute__((packed)) IDT_descriptor_t;

int IDT_init();

#endif
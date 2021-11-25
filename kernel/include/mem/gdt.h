#ifndef _GDT_H_
#define _GDT_H_

#include <stdint.h>

// segment offsets (sizeof(GDT_entry_t) = 8B) 
#define KERNEL_CODE_SEG       0x08
#define KERNEL_DATA_SEG       0x10
#define USERSPACE_CODE_SEG    0x18
#define USERSPACE_DATA_SEG    0x20
#define TSS_SEG               0x28

#define RING_0                0x00
#define RING_3                0x03

#define GDT_ENTRY_COUNT          6

#define GDT_NULL_INDEX           0
#define GDT_KERNEL_CODE_INDEX    1
#define GDT_KERNEL_DATA_INDEX    2
#define GDT_USERSPACE_CODE_INDEX 3
#define GDT_USERSPACE_DATA_INDEX 4
#define GDT_TSS_INDEX            5

// GDT entry structure as described at
// https://wiki.osdev.org/Global_Descriptor_Table
typedef struct {
    uint32_t limit_low   : 16;  // limit [0-15]
    uint32_t base_low    : 16;  // base [0-15]
    uint32_t base_middle : 8;   // base [16-23]
    
    // access byte
    uint32_t AC          : 1;    // 0 by default; CPU sets this to 1 when the segment is accessed
    uint32_t RW          : 1;    // readable/writable (depends on the segment - code/data)
    uint32_t DC          : 1;    // 0 = segment grows up; 1 = segment grown down (if 1 code in this segment can be executed from an equal or lower privilege level)
    uint32_t EX          : 1;    // code segment = 1; data segment = 0
    uint32_t S           : 1;    // code/data segment = 1; TSS (task state segment) = 0
    uint32_t PRIVL       : 2;    // 0 = highest (kernel); 3 = lowest (user applications)
    uint32_t PR          : 1;    // must be 1 for all valid selectors

    uint32_t limit_high  : 4;    // limit [16-19]

    // flags
    uint32_t unused0     : 1;   // unused by default (should be set to 0)
    uint32_t unused1     : 1;   // unused by default (should be set to 0)
    uint32_t SZ          : 1;   // if 0 the selector defines 16 bit protected mode. If 1 it defines 32 bit protected mode. 
    uint32_t GR          : 1;   // if 0 the limit is in 1 B blocks (byte granularity), if 1 the limit is in 4 KiB blocks (page granularity)

    uint32_t base_high   : 8;
} __attribute__((packed)) GDT_entry_t;

// gdt descriptor that will be loaded
// to the CPU using assembly (_load_dgt)
typedef struct {
    uint16_t size;           // limit (size) of the gdt
    uint32_t start_addr;     // address where the gdt starts
} __attribute__((packed)) GDT_descriptor_t;

// TSS (Task State Segment)
// Special data strucutre for multitasking, saves the information about Task (Ragisters)
// https://wiki.osdev.org/Task_State_Segment
// Another info: https://en.wikipedia.org/wiki/Task_state_segment
typedef struct {
    uint32_t link_prev;     // Link to the previous TSS, uses lover 16 bits (upper 16 bits are reserved)
    uint32_t ESP0;          // To load the stack in the kernel mode, 32 bit value
    uint32_t SS0;           // Stack segment to load when in kernel mode, lover 16 bits (upper 16 bits are reserved)
    uint32_t ESP1;          // 32 bit
    uint32_t SS1;           // Lover 16 bits (Upper 16 bits reserved)
    uint32_t ESP2;          // 32 bit
    uint32_t SS2;           // Lover 16 bits (Upper 16 bits reserved)
    uint32_t CR3;           // Should be read only, 32 bit
    uint32_t EIP;           // 32 bit
    uint32_t EFLAGS;        // 32 bit
    uint32_t EAX;           // 32 bit
    uint32_t ECX;           // 32 bit
    uint32_t EDX;           // 32 bit
    uint32_t EBX;           // 32 bit
    uint32_t ESP;           // 32 bit
    uint32_t EBP;           // 32 bit
    uint32_t ESI;           // 32 bit
    uint32_t EDI;           // 32 bit
    uint32_t ES;            // Lover 16 bits (Upper 16 bits reserved)
    uint32_t CS;            // Lover 16 bits (Upper 16 bits reserved)
    uint32_t SS;            // Lover 16 bits (Upper 16 bits reserved)
    uint32_t DS;            // Lover 16 bits (Upper 16 bits reserved)
    uint32_t FS;            // Lover 16 bits (Upper 16 bits reserved)
    uint32_t GS;            // Lover 16 bits (Upper 16 bits reserved)
    uint32_t LDTR;          // Lover 16 bits (Upper 16 bits reserved)
    uint16_t reserved_IOPB; // reserved lover 16 bits of IOPB_offset
    uint16_t IOPB_offset;   // Upper 16 bits (Lover 16 bits reserved)

} __attribute__((packed)) TSS_entry_t;

int GDT_init();

#endif
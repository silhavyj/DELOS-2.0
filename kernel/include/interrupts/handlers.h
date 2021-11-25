#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

#include <stdint.h>

/* This defines what the stack looks like after an ISR was running */
typedef struct
{
    uint32_t EAX_old;
    uint32_t gs, fs, es, ds;                            /* pushed the segs last */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;    /* pushed by 'pusha' */
    uint32_t int_no, err_code;                           /* our 'push #' and ecodes do this */
    uint32_t eip, cs, eflags, useresp, ss;              /* pushed by the processor automatically */
}__attribute__((packed)) Interrupt_generic_registers_t;

// interrupt handlers call from the assembly language
extern "C" {
    void _generic_interrupt_handler(Interrupt_generic_registers_t regs);  // generic one
    void _int0xE_handler(uint32_t pfla);                                  // Handler for Page Fault, only want the PFLA (Page Fault Linear Address = 32 bit)
};

#endif

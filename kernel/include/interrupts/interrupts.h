#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

// https://wiki.osdev.org/Exceptions
// https://wiki.osdev.org/Interrupts
// https://www.youtube.com/watch?v=AgeX-U4dKSs&list=PLHh55M_Kq4OApWScZyPl5HhgsTJS9MZ6M&index=6

// interrupt handlers written in assembly
// the reason for this is to avoid optimizer corrupting the function
// https://wiki.osdev.org/Interrupt_Service_Routines
extern "C" {
    void _isr8();  // Double Fault
    void _isrA();  // Invalid TSS
    void _isrB();  // Segment Not Present
    void _isrC();  // Stack-Segment Fault
    void _isrD();  // General Protection Fault
    void _isrE();  // Page Fault
    void _isrF();  // Reserved
    void _isr10();  // x87 Floating-Point Exception
    void _isr13();  // SIMD Floating-Point Exception
    void _isr20();  // PIT (system timer)
    void _isr21();  // keyboard
    void _isr80();  // system calls
    void _isr2C();  // system calls
}

#endif

#ifndef _IRQ_H_
#define _IRQ_H_

// https://wiki.osdev.org/PIC (protected mode)
// https://web.fe.up.pt/~pfs/aulas/lcom2020/labs/lab2/lab2_04.html

#define PIC1        0x20 // IO base address for master PIC
#define PIC1_CMD    PIC1
#define PIC1_DATA   (PIC1 + 1)
#define PIC1_OFFSET 0x20

#define PIC2        0xA0 // IO base address for slave PIC
#define PIC2_CMD    PIC2
#define PIC2_DATA   (PIC2 + 1)
#define PIC2_OFFSET 0x28
 
#define PIC_EOI     0x20  // end of interrupt (acknowledge)

#define ICW1_INIT   0x10  // Initialization - required!
#define ICW1_ICW4   0x01  // ICW4 (not) needed
#define ICW4_8086   0x01  // 8086/88 (MCS-80/85) mode

#define PIT_IRQ      0x00 // PIT is hooked up to IRQ0
#define PS2_KEYBOARD 0x01 // PS2 keyboard is hooked up to IRQ1

#define PIC1_IRQ_ACK 0x00 // acknowledge for PIC1
#define PIC2_IRQ_ACK 0x08 // acknowledge for PIC2

void PIC_sendEOI(unsigned char irq);
int PIC_remap();

#endif
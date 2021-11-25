#include <interrupts/irq.h>
#include <common.h>

void PIC_sendEOI(unsigned char irq) {
    if (irq >= 8) {
        _outb(PIC2_CMD, PIC_EOI);
    } else {
        _outb(PIC1_CMD, PIC_EOI);
    }
}

// reinitialize the PIC controllers, giving them specified vector
// offsets rather than 8h and 70h, as configured by default
int PIC_remap() {
    // save masks
    unsigned char PIC1_mask = _inb(PIC1_DATA);
    unsigned char PIC2_mask = _inb(PIC2_DATA);

    // starts the initialization sequence (in cascade mode)
    _outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    _io_wait();
    _outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    _io_wait();

    // set new vector offsets for both PICs
    _outb(PIC1_DATA, PIC1_OFFSET); // ICW2: Master PIC vector offset
    _io_wait();
    _outb(PIC2_DATA, PIC2_OFFSET); // ICW2: Slave PIC vector offset
    _io_wait();

    // acknowledge the other PIC so the Master PIC knows about the Slave PIC
    _outb(PIC1_DATA, 4);           // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    _io_wait();
    _outb(PIC2_DATA, 2);           // ICW3: tell Slave PIC its cascade identity (0000 0010)
    _io_wait();

    // set 8086/88 mode
    _outb(PIC1_DATA, ICW4_8086);
    _io_wait();
    _outb(PIC2_DATA, ICW4_8086);
    _io_wait();

    // restore saved masks.
    _outb(PIC1_DATA, PIC1_mask);
    _outb(PIC2_DATA, PIC2_mask);

    return 0;
}
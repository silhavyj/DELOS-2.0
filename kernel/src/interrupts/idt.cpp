#include <interrupts/idt.h>
#include <mem/gdt.h>
#include <common.h>
#include <interrupts/interrupts.h>

IDT_descriptor_t idt_desc;
IDT_gate_t idt_gates[IDT_GATE_COUNT];

static void set_idt_gate(uint8_t index, uint32_t handler_addr, uint16_t segment, uint8_t present, uint8_t DPL, uint8_t storage_segment, uint8_t type);

int IDT_init() {
    int index;

    // set all gates as unused by default
    // we'll set the particular gates we want to have active individually
    for (index = 0; index < IDT_GATE_COUNT; index++)
        set_idt_gate(index, 0, 0, IDT_NOT_PRESENT, 0, 0, IDT_32_BIT_TRAP_GATE);

    // set handlers for the particular interrupts
    // the functions that accept the interrupt are written in assembly,
    // however, from there, we jump back to c++ (handlers.h/cpp)
    set_idt_gate(0x08, reinterpret_cast<uint32_t>(&_isr8), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // Double Fault
    set_idt_gate(0x0A, reinterpret_cast<uint32_t>(&_isrA), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // Invalid TSS
    set_idt_gate(0x0B, reinterpret_cast<uint32_t>(&_isrB), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // Segment Not Present
    set_idt_gate(0x0C, reinterpret_cast<uint32_t>(&_isrC), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // Stack-Segment Fault
    set_idt_gate(0x0D, reinterpret_cast<uint32_t>(&_isrD), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // General Protection Fault
    set_idt_gate(0x0E, reinterpret_cast<uint32_t>(&_isrE), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // Page Fault
    set_idt_gate(0x0F, reinterpret_cast<uint32_t>(&_isrF), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // Reserved
    set_idt_gate(0x10, reinterpret_cast<uint32_t>(&_isr10), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // x87 Floating-Point Exception
    set_idt_gate(0x13, reinterpret_cast<uint32_t>(&_isr13), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // SIMD Floating-Point Exception
    set_idt_gate(0x20, reinterpret_cast<uint32_t>(&_isr20), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // PIT (system timer)
    set_idt_gate(0x21, reinterpret_cast<uint32_t>(&_isr21), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // keyboard
    set_idt_gate(0x80, reinterpret_cast<uint32_t>(&_isr80), KERNEL_CODE_SEG, IDT_PRESENT, 3, 0, IDT_32_BIT_INTERRUPT_GATE); // system calls
    set_idt_gate(0x2C, reinterpret_cast<uint32_t>(&_isr2C), KERNEL_CODE_SEG, IDT_PRESENT, 0, 0, IDT_32_BIT_INTERRUPT_GATE); // system calls

    // initialize idt descriptors (size and address)
    idt_desc.limit = sizeof(idt_gates);
    idt_desc.base  = reinterpret_cast<uint32_t>(&idt_gates);

    // load descriptor to the CPU
    _load_idt(reinterpret_cast<uint32_t>(&idt_desc));

    return 0; // IDT has been loaded successfully
}

void set_idt_gate(uint8_t index, uint32_t handler_addr, uint16_t segment, uint8_t present, uint8_t DPL, uint8_t storage_segment, uint8_t type) {
    idt_gates[index].offset_low  = handler_addr & 0xFFFF;  // lower 16 bits of the address off the handler function
    idt_gates[index].selector    = segment;                // code segment selector in GDT or LDT
    idt_gates[index].zero        = 0;                      // unused
    idt_gates[index].gate_type   = type & 0b1111;          // gate type (4 bits)
    idt_gates[index].S           = storage_segment & 0b1;  // Storage Segment; set to 0 for interrupt and trap gates
    idt_gates[index].DPL         = DPL & 0b11;             // Descriptor Privilege Level (2 bits)
    idt_gates[index].P           = present & 0b1;          // set the gate as present (1 bit)
    idt_gates[index].offset_high = handler_addr >> 16;     // higher 16 bits of the address off the handler function
}
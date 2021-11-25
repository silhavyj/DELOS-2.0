#include <interrupts/irq.h>
#include <interrupts/handlers.h>
#include <common.h>
#include <drivers/screen/screen.h>
#include <drivers/screen/color.h>
#include <drivers/keyboard/keyboard.h>
#include <drivers/mouse/mouse.h>
#include <processes/process.h>
#include <processes/scheduler.h>
#include <processes/syscalls.h>

#pragma GCC diagnostic ignored "-Wunused-parameter"

static void kill_running_process() {
    PCB_t *running_process = get_running_process();
    wake_up_parent_process(running_process->ppid, 1);
    kill_process(running_process);
    switch_to_next_process();
}

// Double Fault Exception interrupt handler
static void int0x8_handler(Interrupt_generic_registers_t *regs) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt Double Fault Exception!\r\n");
    reset_color();
    kill_running_process();
}

// Invalid TSS interrupt handler
static void int0xA_handler(Interrupt_generic_registers_t *regs) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt Invalid TSS!\r\n");
    reset_color();
    _panic();
}

// Segment not present interrupt handler
static void int0xB_handler(Interrupt_generic_registers_t *regs) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt Segment not present!\r\n");
    reset_color();
    kill_running_process();
}

// Stack segment fault interrupt handler
static void int0xC_handler(Interrupt_generic_registers_t *regs) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt Stack segment fault!\r\n");
    reset_color();
    kill_running_process();
}

// General protection fault interrupt handler
static void int0xD_handler(Interrupt_generic_registers_t *regs) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt GPF! (General Protection Fault)\r\n");
    reset_color();
    kill_running_process();
}

// Unknown interrupt handler
static void int0xF_handler(Interrupt_generic_registers_t *regs) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt Unknown\r\n");
    kill_running_process();
}

// Coprocessor fault interrupt handler
static void int0x10_handler(Interrupt_generic_registers_t *regs) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt Coprocessor fault\r\n");
    reset_color();
    _panic();
}

// SIMD Floating-Point Exception interrupt handler
static void int0x13_handler(Interrupt_generic_registers_t *regs) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt SIMD Floating-Point Exception!\r\n");
    reset_color();
    kill_running_process();
}

// Keyboard interrupt handler
static void int0x21_handler(Interrupt_generic_registers_t *regs) {
    uint8_t scancode = _inb(KEYBOARD_DATA_PORT);
    process_key(scancode);

    // PIC is waiting for us to let him know once
    // we're done handling the interrupt
    // by default PIT is connected via IRQ0
    PIC_sendEOI(PS2_KEYBOARD);
}

// Syscall interrupt handler
static void int0x80_handler(Interrupt_generic_registers_t *regs) {
    PCB_t *running_process = get_running_process();
    save_process_context(running_process, regs);
    sys_callback();
    switch_to_next_process();
}

// Page fault explicit interrupt handler
void _int0xE_handler(uint32_t pfla) {
    set_color(FOREGROUND_YELLOW);
    kprintf("Interrupt Page Fault, on address: 0x%x\r\n", pfla);
    reset_color();
    kill_running_process();
}

// PIT explicit interrupt handler
void int0x20_handler(Interrupt_generic_registers_t *regs) {
    // switch context every N ticks
    static int ticks = 0;
    if (++ticks % TICKS_FOR_TASK_SWITCH == 0) {
        ticks = 0;
    } else {
        PIC_sendEOI(PIT_IRQ);
        return;
    }
    PCB_t *running_process = get_running_process();
    save_process_context(running_process, regs);
    set_process_as_ready(running_process);
    PIC_sendEOI(PIT_IRQ);
    switch_to_next_process();
}

static void int0x2C_handler(Interrupt_generic_registers_t *regs) {
    mouse_callback();
    PIC_sendEOI(PIC1_IRQ_ACK);
    PIC_sendEOI(PIC2_IRQ_ACK);
}

//Generic interrupt handler
void _generic_interrupt_handler(Interrupt_generic_registers_t regs) {
    switch (regs.int_no) {
        case 0x8:
            int0x8_handler(&regs);
            break;
        case 0xA:
            int0xA_handler(&regs);
            break;
        case 0xB:
            int0xB_handler(&regs);
            break;
        case 0xC:
            int0xC_handler(&regs);
            break;
        case 0xD:
            int0xD_handler(&regs);
            break;
        case 0xF:
            int0xF_handler(&regs);
            break;
        case 0x10:
            int0x10_handler(&regs);
            break;
        case 0x13:
            int0x13_handler(&regs);
            break;
        case 0x20:
            int0x20_handler(&regs);
            break;
        case 0x21:
            int0x21_handler(&regs);
            break;
        case 0x80:
            int0x80_handler(&regs);
            break;
        case 0x2C:
            int0x2C_handler(&regs);
            break;
        default:
            //NO IDEA
            set_color(FOREGROUND_RED);
            kprintf("Interrupt handler not found!\r\n");
            reset_color();
            _panic();
            break;
    }
}
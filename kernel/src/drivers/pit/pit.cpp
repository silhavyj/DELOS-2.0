#include <drivers/pit/pit.h>
#include <stdint.h>
#include <common.h>

int PIT_init() {
    uint32_t divisor = 1193180 / FREQUENCY;  // Calculate our divisor 1.19MHz (1193180Hz)
    _outb(PIT_CMD, 0x36);                    // Set our command byte 0x36 (RW + square wave)
    _outb(PIT0_DATA, divisor & 0xFF);        // Set low byte of divisor
    _outb(PIT0_DATA, divisor >> 8);          // Set high byte of divisor

    return 0;
}
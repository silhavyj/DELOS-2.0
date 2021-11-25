#ifndef _MOUSE_H_
#define _MOUSE_H_

#include <stdint.h>

// Port definitions (https://wiki.osdev.org/PS/2_Mouse)
#define MOUSE_INTERRUPT_SIGNAL 0x2C
#define MOUSE_DATA_PORT 0x60
#define MOUSE_COMMAND_PORT 0x64

#define MOUSE_ACTIVATE_INTERRUPTS 0xA8
#define MOUSE_GET_CURRENT_STATE 0x20
#define MOUSE_SET_STATE 0x60

#define MOUSE_BUFFER_SIZE 3

// Default cursor position
#define DEFAULT_X 40
#define DEFAULT_Y 12

int mouse_init();

void mouse_callback();
uint8_t get_mouse_y_pos();
uint8_t get_mouse_x_pos();
uint8_t is_mouse_disabled();
void set_first_invert_after_enabled(uint8_t value);

#endif
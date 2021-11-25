#include <drivers/screen/screen.h>
#include <drivers/mouse/mouse.h>
#include <memory.h>
#include <common.h>

// Buffer for accepted values
static uint8_t buffer[MOUSE_BUFFER_SIZE];

// Current buffer position
static uint8_t offset;
static uint8_t buttons;
static uint8_t mouse_disabled;
static int8_t x = DEFAULT_X;
static int8_t y = DEFAULT_Y;
static uint8_t first_invert_after_enabled = 0;

static void print_mouse_state(uint8_t disabled);

uint8_t get_mouse_y_pos() {
    return y;
}

uint8_t get_mouse_x_pos() {
    return x;
}

uint8_t is_mouse_disabled() {
    return mouse_disabled;
}

void set_first_invert_after_enabled(uint8_t value) {
    first_invert_after_enabled = value;
}

// Mouse initialization
int mouse_init() {
    // Init default flag values
    offset = 0;
    buttons = 0;
    mouse_disabled = 0;
    memset(buffer, 0, MOUSE_BUFFER_SIZE);

    // Init cursor
    invert_colors(DEFAULT_Y, DEFAULT_X);

    // Init mouse ports
    _outb(MOUSE_COMMAND_PORT, MOUSE_ACTIVATE_INTERRUPTS);
    _outb(MOUSE_COMMAND_PORT, MOUSE_GET_CURRENT_STATE);
    uint8_t status = _inb(MOUSE_DATA_PORT) | 2;
    _outb(MOUSE_COMMAND_PORT, MOUSE_SET_STATE);
    _outb(MOUSE_DATA_PORT, status);
    
    _outb(MOUSE_COMMAND_PORT, 0xD4);
    _outb(MOUSE_DATA_PORT, 0xF4);
    _inb(MOUSE_DATA_PORT);

    return 0;
}

// Mouse interrupt handler
void mouse_callback() {
    uint8_t status = _inb(MOUSE_COMMAND_PORT);
    // If there is anything to read
    if(!(status & MOUSE_GET_CURRENT_STATE)) {
        return;
    }

    static uint8_t button_down = 0;
    static uint8_t erase_last_active_pos = 0;

    // Then read into buffer
    buffer[offset] = _inb(MOUSE_DATA_PORT);

    // And set offset to next position
    offset = (offset + 1) % 3;

    // Data can be read
    if (offset == 0) {
        int8_t prev_y = y;
        int8_t prev_x = x;

        // Positions are relative
        x += buffer[1];
        if (x < 0) x = 0;
        if (x >= MAX_COLS) x = MAX_COLS - 1;

        y -= buffer[2];
        if (y < 0) y = 0;
        if (y >= MAX_ROWS) y = MAX_ROWS - 1;

        // Mouse buttons pressing
        for (uint8_t i = 0; i < 3; i++) {
            if ((buffer[0] & (0x01 << i)) != (buttons & (0x01 << i))) {
                if (button_down == 0) {
                    mouse_disabled = !mouse_disabled;
                    button_down = 1;
                    erase_last_active_pos = 1;
                } else {
                    button_down = 0;
                }
            }
        }

        // Mouse move & mouse click cursor rendering
        if (mouse_disabled == 0) {
            if (first_invert_after_enabled == 1) {
                invert_colors(prev_x, prev_y);
            }
            invert_colors(x, y);
            first_invert_after_enabled = 1;
        } else if (erase_last_active_pos == 1) {
            invert_colors(prev_x, prev_y);
            erase_last_active_pos = 0;
            first_invert_after_enabled = 0;
            print_mouse_state(mouse_disabled);
        }
        buttons = buffer[0];
    }
}

// Printing mouse click state
static void print_mouse_state(uint8_t disabled) {
    set_color(FOREGROUND_CYAN);
    print_string("MOUSE ");
    if (disabled == 0) {
        print_string("ENABLED\n\r");
    } else {
        print_string("DISABLED\n\r");
    }
    reset_color();
}
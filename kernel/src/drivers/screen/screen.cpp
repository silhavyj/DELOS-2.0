#include <drivers/screen/screen.h>
#include <drivers/mouse/mouse.h>
#include <common.h>
#include <string.h>
#include <memory.h>
#include <processes/scheduler.h>

// Screen buffer
static char screen_buffer[SCREEN_BUFFER_SIZE];
uint8_t screen_color;

// Sets cursor position
void set_cursor(uint32_t offset) {
    offset /= 2;
    _outb(VGA_CTRL_REG, VGA_OFFSET_HIGH);
    _outb(VGA_DATA_REG, (unsigned char)(offset >> 8));
    _outb(VGA_CTRL_REG, VGA_OFFSET_LOW);
    _outb(VGA_DATA_REG, (unsigned char)(offset & 0xFF));
}

// Return cursor position
static uint32_t get_cursor() {
    _outb(VGA_CTRL_REG, VGA_OFFSET_HIGH);
    uint32_t offset = _inb(VGA_DATA_REG) << 8;
    _outb(VGA_CTRL_REG, VGA_OFFSET_LOW);
    offset += _inb(VGA_DATA_REG);
    return offset * 2;
}

void print_terminal_index(int index) {
    char terminal_indicator[] = "TERMINAL ?";
    int len = strlen(terminal_indicator);
    terminal_indicator[len - 1] = ('0' + index);

    uint32_t cursor = get_cursor();
    set_cursor(get_offset(MAX_COLS - len, 0));
    print_string(terminal_indicator);
    set_cursor(cursor);
}

// Sets char in position offset in screen memory
static void set_char(char c, uint32_t offset) {
    unsigned char* memory = (unsigned char*)VIDEO_ADDRESS;
    memory[offset] = c;
    memory[offset + 1] = screen_color;
}

// Handler for scrolling (copies MAX_ROWS - 1 row & creates new empty line at the bottom of the screen)
static uint32_t scroll_handler(uint32_t offset) {
    uint32_t col;
    memcpy(
            (char*)(get_offset(0, 0) + VIDEO_ADDRESS),
            (char*)(get_offset(0, 1) + VIDEO_ADDRESS),
            MAX_COLS * (MAX_ROWS - 1) * 2
    );

    for (col = 0; col < MAX_COLS; col++) {
        set_char(' ', get_offset(col, MAX_ROWS - 1));
    }
    return offset - 2 * MAX_COLS;
}

// Printf implementation (what else to say)
void kprintf(char *c, ...) {
    memset(screen_buffer, 0, SCREEN_BUFFER_SIZE);
    char convertor_buffer[CONVERTOR_BUFFER_SIZE];
    va_list lst;
    va_start(lst, c);
    while(*c != '\0')
    {
        if(*c != '%')
        {
            append(screen_buffer, *c);
            c++;
            continue;
        }

        c++;

        if(*c == '\0')
        {
            break;
        }

        switch(*c)
        {
            case 'c':
			    append(screen_buffer, (char)va_arg(lst, int));
                break;
            case 's': 
                strcat(screen_buffer, va_arg(lst, char*));
                break;
            case 'd': 
                strcat(screen_buffer, itoa_dec(convertor_buffer, va_arg(lst, int)));
                break;
            case 'x':
                strcat(screen_buffer, itoa_hex(convertor_buffer, va_arg(lst, int)));
                break;
            case 'f':
                break;
            default:
                append(screen_buffer, '%');
                break;
        }
        c++;
    }
    va_end(lst);
    if(is_scheduler_initialized()){
        PCB_t *running_process = get_running_process();
        if(running_process->shell_id == get_focused_terminal()){
            print_string(screen_buffer);
        }
        if(running_process->state != PROCESS_STATE_TERMINATION){
            print_to_stream(running_process, screen_buffer);
        }
    }
    else{
        print_string(screen_buffer);
    }
}

// Prints string to current cursor position
void print_string(const char* string) {
    uint32_t offset = get_cursor();
    uint32_t i = 0;
    static uint8_t prev_mouse_x_pos = 0;
    static uint8_t prev_mouse_y_pos = 0;

    while(string[i] != 0) {
        if (offset >= MAX_ROWS * MAX_COLS * 2) {
            offset = scroll_handler(offset);

            uint8_t y = get_mouse_y_pos();
            uint8_t x = get_mouse_x_pos();
            uint8_t mouse_disabled = is_mouse_disabled();

            if (mouse_disabled == 0 && (y != prev_mouse_y_pos || x != prev_mouse_x_pos)) {
                invert_colors(x, y - 1);
                set_first_invert_after_enabled(0);
            }
            prev_mouse_x_pos = x;
            prev_mouse_y_pos = y;
        }
        if(string[i] == '\n') {
            offset = new_line_offset(offset);
        } else if(string[i] == '\r') {
            offset = start_line_offset(offset);
        } else {
            set_char(string[i], offset);
            offset += 2;
        }
        i++;
    }
    set_cursor(offset);
}

// Backspace printer
void print_backspace() {
    uint32_t cursor = get_cursor() - 2;
    set_char(' ', cursor);
    set_cursor(cursor);
}

// Newline printer
void print_new_line() {
    uint32_t cursor = get_cursor();
    cursor = new_line_offset(cursor);
    cursor = start_line_offset(cursor);
    set_cursor(cursor);
}

uint32_t get_row(const uint32_t offset) {
    return offset / (2 * MAX_COLS);
}

uint32_t get_column(const uint32_t offset) {
    return (offset - get_row(offset) * 2 * MAX_COLS) / 2;
}

uint32_t get_offset(const uint32_t column, const uint32_t row) {
    return 2 * (row * MAX_COLS + column);
}

uint32_t new_line_offset(const uint32_t offset) {
    return get_offset(get_column(offset), get_row(offset) + 1);
}

uint32_t start_line_offset(const uint32_t offset) {
    return get_offset(0, get_row(offset));
}

// Inverting cell color (background & foreground)
void invert_colors(uint8_t column, uint8_t row) {
    unsigned char* memory = (unsigned char*)VIDEO_ADDRESS;
    uint32_t offset = get_offset(column, row);
    char color = memory[offset + 1];
    char loNibble = (color & 0xf0) >> 4;
    char hiNibble = (color & 0x0f) << 4;
    memory[offset + 1] = hiNibble | loNibble;
}

void set_color(uint8_t val) {
    screen_color = val;
}

void reset_color() {
    screen_color = DEFAULT_COLOR;
}

// Clears the screen
void clear_screen() {
    uint32_t i;
    reset_color();
    for(i = 0; i < MAX_COLS * MAX_ROWS; i++) {
        set_char(' ', i * 2);
    }
    set_cursor(get_offset(0, 0));
}
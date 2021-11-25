#include <drivers/keyboard/keyboard.h>
#include <drivers/keyboard/keymap.h>
#include <drivers/screen/screen.h>
#include <drivers/keyboard/ctrl_keyboard_shorcuts.h>
#include <memory.h>
#include <processes/scheduler.h>
#include <interrupts/irq.h>

// Keyboard buffer
char keyboard_buffer[KEYBOARD_BUFF_SIZE];
uint8_t keyboard_buff_pos = 0;

// User input history buffer
cmd_history_record_t history_buffer[MAX_CMD_HISTORY_RECORDS];
uint8_t history_buff_pos = 0;

// Key flags
uint8_t shift_on    = 0;
uint8_t ctrl_on     = 0;
uint8_t alt_on      = 0;
uint8_t capslock_on = 0;

void abort_current_cmd() {
    PCB_t *running_process = get_latest_running_non_idle_process();
    if (running_process->pid == 0 ||
        running_process->pid == 1 ||
        running_process->pid == 2 ||
        running_process->pid == 3 ||
        running_process->pid == 4 ) {
        //kprintf("^C\n\r");
        print_string("^C\n\r");
        keyboard_buff_pos = 0;
    }
    else {
        wake_up_parent_process(running_process->ppid, 1);
        PCB_t *curr_on_cpu = get_running_process();
        if (curr_on_cpu != running_process){
            set_process_as_ready(curr_on_cpu);
        }
        kill_process(running_process);
        //kprintf("^C\n\r");
        print_string("^C\n\r");
        keyboard_buff_pos = 0;
        PIC_sendEOI(PS2_KEYBOARD);
        switch_to_next_process();
    }
}

// Handling user input & identifies symbols (sets flags or returns specific symbol from keymap)
static unsigned char convert_scan_code(uint8_t scan_code, uint8_t *pressed) {
    int original_scancode = scan_code & KEY_CODE_MASK;
    *pressed = (scan_code & KEY_RELEASED) ? 1 : 0;

    // Flag set
    switch (original_scancode) {
        case KEY_CODE_LEFT_SHIFT:
        case KEY_CODE_RIGHT_SHIFT:
            shift_on = !(*pressed);
            return 0;
        case KEY_CODE_CTRL:
            ctrl_on = !(*pressed);
            return 0;
        case KEY_CODE_ALT:
            alt_on = !(*pressed);
            return 0;
        case KEY_CODE_CAPSLOCK:
            if (*pressed)
                capslock_on = !capslock_on;
            return 0;
        case KEY_CODE_BACK_SPACE:
            if (*pressed)
                return KEY_CODE_BACK_SPACE;
            return 0;
        case KEY_CODE_LEFT_ARROW:
        case KEY_CODE_RIGHT_ARROW:
        case KEY_CODE_UP_ARROW:
        case KEY_CODE_DOWN_ARROW:
            return 0;
        case ENTER_KEY_CODE:
            if (*pressed)
                return ENTER_KEY_CODE;
            return 0;
    }
    // Handler for CTRL shortcuts
    if (ctrl_on && !shift_on && (*pressed)) {
        crl_keyboard_shortcut_t *ctr_short_cut = get_ctrl_shortcut(key_map[original_scancode]);
        if (ctr_short_cut != NULL)
            ctr_short_cut->handler();
        return KEY_CODE_BACK_SPACE; // So the letter doesn't get displayed
    }
    // Key identification
    if (shift_on) {
        if (capslock_on)
            return key_map_caps_shifted[original_scancode];
        return key_map_shifted[original_scancode];
    }
    if (capslock_on)
        return key_map_caps_normal[original_scancode];
    return key_map[original_scancode];
}

// Keyboard interrupt handler (key forwarding for identification & printing)
void process_key(uint8_t scan_code) {
    uint8_t pressed;
    char symbol = convert_scan_code(scan_code, &pressed);
    char temp_buff[3];

    if (symbol == KEY_CODE_BACK_SPACE) {
        if (keyboard_buff_pos > 0) {
            print_backspace();
            keyboard_buff_pos--;
        }
    } else if (symbol == ENTER_KEY_CODE) {
        temp_buff[0] = '\r';
        temp_buff[1] = '\n';
        temp_buff[2] = '\0';
        //kprintf("\n\r");
        print_string(temp_buff);
        keyboard_buffer[keyboard_buff_pos] = '\0';
        wake_process_waiting_for_keyboard(keyboard_buffer);
        keyboard_buff_pos = 0;
    }
    // Symbol is printable (just print it)
    else if (pressed && symbol != 0) {
        temp_buff[0] = symbol;
        temp_buff[1] = '\0';
        temp_buff[2] = '\0';
        //kprintf("%c", symbol);
        print_string(temp_buff);
        keyboard_buffer[keyboard_buff_pos++] = symbol;
        if (keyboard_buff_pos == KEYBOARD_BUFF_SIZE)
            keyboard_buff_pos = 0;
    }
}

// Initializes keyboard driver & clears keyboard buffer
int keyboard_init() {
    memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
    return 0;
}

void switch_to_terminal_1() {
    switch_to_terminal(1);
}

void switch_to_terminal_2() {
    switch_to_terminal(2);
}

void switch_to_terminal_3() {
    switch_to_terminal(3);
}

void switch_to_terminal_4() {
    switch_to_terminal(4);
}
#include <drivers/screen/screen.h>
#include <stdint.h>
#include <drivers/keyboard/keyboard.h>
#include <drivers/keyboard/ctrl_keyboard_shorcuts.h>

// Custom CTRL shortcuts implementation
crl_keyboard_shortcut_t ctr_keyboard_shortcuts[] = {
    {'l', &clear_screen         },
    {'c', &abort_current_cmd    },
    {'1', &switch_to_terminal_1 },
    {'2', &switch_to_terminal_2 },
    {'3', &switch_to_terminal_3 },
    {'4', &switch_to_terminal_4 },
};

int ctr_shortcuts_count = sizeof(ctr_keyboard_shortcuts) / sizeof(crl_keyboard_shortcut_t);

// Identifies used shortcut
crl_keyboard_shortcut_t *get_ctrl_shortcut(char letter) {
    int i;
    for (i = 0; i < ctr_shortcuts_count; i++)
        if (ctr_keyboard_shortcuts[i].letter == letter)
            return &ctr_keyboard_shortcuts[i];
    return NULL;
}
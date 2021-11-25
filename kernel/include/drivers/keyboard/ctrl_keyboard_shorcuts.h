#ifndef CTRL_KEYBOARD_SHORTCUTS_H_
#define CTRL_KEYBOARD_SHORTCUTS_H_

// Structure definition for custom CTRL keyboard shortcuts
typedef struct {
    char letter; // CTRL^<letter>
    void(*handler)(void); // Pointer of function to be called
} crl_keyboard_shortcut_t;

// Identifies used shortcut
crl_keyboard_shortcut_t *get_ctrl_shortcut(char letter);

#endif
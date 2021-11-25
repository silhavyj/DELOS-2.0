#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdint.h>
#include <stdarg.h>
#include <drivers/screen/color.h>

// Buffer sizes
#define SCREEN_BUFFER_SIZE 256
#define CONVERTOR_BUFFER_SIZE 64

// Video memory start adress range
#define VIDEO_ADDRESS 0xB8000

// Screen size
#define MAX_ROWS 25
#define MAX_COLS 80

// Screen device I/O ports
#define VGA_CTRL_REG 0x3D4
#define VGA_DATA_REG 0x3D5
#define VGA_OFFSET_LOW 0x0F
#define VGA_OFFSET_HIGH 0x0E

// Default color definition
#define DEFAULT_COLOR FOREGROUND_WHITE | BACKGROUND_BLACK
#define ERROR_COLOR FOREGROUND_WHITE | BACKGROUND_RED

// Cursor manipulation
void kprintf(char *c, ...);
void print_string(const char* string);
void print_backspace();
void print_new_line();

uint32_t get_row(const uint32_t offset);
uint32_t get_column(const uint32_t offset);
uint32_t get_offset(const uint32_t column, const uint32_t row);
uint32_t new_line_offset(const uint32_t offset);
uint32_t start_line_offset(const uint32_t offset);
void print_terminal_index(int index);

void invert_colors(uint8_t y, uint8_t x);
void set_color(uint8_t val);
void set_cursor(uint32_t offset);
void reset_color();
void clear_screen();

#endif

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <stdint.h>

#define PRINT_BUFF_SIZE 256

void printf(const char *str, ...);

extern "C" {
    void exit(int exit_code);
    void *malloc(uint32_t size);
    void free(void *addr);
    int get_pid();
    int get_ppid();
    int exec(const char *program);
    int open(const char *filename);
    int close(const char *filename);
    int read(char *filename, char *buffer, uint32_t offset, uint32_t len);
    int write(char *filename, char *buffer, uint32_t offset, uint32_t len);
    int touch(const char *filename);
    void ls();
    int cat(const char *filename);
    int rm(const char *filename);
    int cp(const char *filename1 , const char *filename2);
    void ps();
    void lp();
    int fork();
    int wait_for_child(int pid);
    int get_last_process_return_value();
    void clear_screen_command();
    void read_line(char *buffer);
    void file_append(char *filename, char *buffer);
    void color_screen_command(uint32_t foreground, uint32_t background);
    void color_screen_command(uint32_t foreground, uint32_t background);
    void set_cursor_command(uint32_t xAxis, uint32_t yAxis);
}

#endif
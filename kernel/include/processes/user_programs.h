#ifndef _USER_PROGRAMS_H_
#define _USER_PROGRAMS_H_

#include <stdint.h>

#define PROGRAM_NAME_LEN 16

typedef struct {
    char name[PROGRAM_NAME_LEN];
    char *code;
    uint32_t size;
} program_t;

program_t *get_program(const char *name);
void print_all_programs();

#endif
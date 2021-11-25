#include <processes/user_programs.h>
#include <drivers/screen/screen.h>
#include <string.h>

#include "../../userspace/programs/idle.bin.h"
#include "../../userspace/programs/test_wait.bin.h"
#include "../../userspace/programs/test_malloc.bin.h"
#include "../../userspace/programs/test_fork.bin.h"
#include "../../userspace/programs/shell.bin.h"
#include "../../userspace/programs/calc.bin.h"
#include "../../userspace/programs/ipcalc.bin.h"
#include "../../userspace/programs/page_fault.bin.h"
#include "../../userspace/programs/gpf.bin.h"
#include "../../userspace/programs/zero.bin.h"
#include "../../userspace/programs/yell_A.bin.h"
#include "../../userspace/programs/yell_B.bin.h"
#include "../../userspace/programs/par_demo.bin.h"
#include "../../userspace/programs/par_demo2.bin.h"
#include "../../userspace/programs/hanoi.bin.h"
#include "../../userspace/programs/fibonacci.bin.h"
#include "../../userspace/programs/rain.bin.h"

static program_t programs[] = {
    { "idle.exe",        (char *)idle_bin, idle_bin_len               },
    { "test_fork.exe",   (char *)test_fork_bin, test_fork_bin_len     },
    { "test_malloc.exe", (char *)test_malloc_bin, test_malloc_bin_len },
    { "test_wait.exe",   (char *)test_wait_bin, test_wait_bin_len     },
    { "shell.exe",       (char *)shell_bin, shell_bin_len             },
    { "calc.exe",        (char *)calc_bin, calc_bin_len               },
    { "ipcalc.exe",      (char *)ipcalc_bin, ipcalc_bin_len           },
    { "page_fault.exe",  (char *)page_fault_bin, page_fault_bin_len   },
    { "gpf.exe",         (char *)gpf_bin, gpf_bin_len                 },
    { "zero.exe",        (char *)zero_bin, zero_bin_len               },
    { "yell_A.exe",      (char *)yell_A_bin, yell_A_bin_len           },
    { "yell_B.exe",      (char *)yell_B_bin, yell_B_bin_len           },
    { "par_demo.exe",    (char *)par_demo_bin, par_demo_bin_len       },
    { "par_demo2.exe",   (char *)par_demo2_bin, par_demo2_bin_len     },
    { "hanoi.exe",       (char *)hanoi_bin, hanoi_bin_len             },
    { "fibonacci.exe",   (char *)fibonacci_bin, fibonacci_bin_len     },
    { "rain.exe",        (char *)rain_bin, rain_bin_len     },

};

static uint32_t program_count = sizeof(programs) / sizeof(program_t);

program_t *get_program(const char *name) {
    uint32_t i;
    for (i = 0; i < program_count; i++) {
        if (strcmp(name, programs[i].name) == 0)
            return &programs[i];
    }
    return NULL;
}

void print_all_programs() {
    uint32_t i;
    for (i = 0; i < program_count; i++) {
        kprintf("%s\n\r", programs[i].name);
    }
}
#include <processes/syscalls.h>
#include <drivers/screen/screen.h>
#include <processes/scheduler.h>
#include <processes/process.h>
#include <processes/user_programs.h>
#include <common.h>
#include <fs/vfs.h>
#include <string.h>
#include <memory.h>

// #define DEBUG_PIDS

static int last_exit_code = 0;

static void sys_call_exit(PCB_t *pcb) {
    _load_page_dir(PAGE_DIR_ADDR);
    last_exit_code = pcb->regs.ebx;
    wake_up_parent_process(pcb->ppid, pcb->regs.ebx);
    kill_process(pcb);
}

void sys_call_printf(PCB_t *pcb) {
    #ifdef DEBUG_PIDS
        set_color(FOREGROUND_DARKGRAY);
        kprintf("%s (pid=%d | ppid=%d): ", pcb->name, pcb->pid, pcb->ppid);
        reset_color();
    #endif
    kprintf("%s", (char *) pcb->regs.esi);
    set_process_as_ready(pcb);
}

static void sys_call_malloc(PCB_t *pcb) {
    _load_page_dir(pcb->regs.cr3);
    pcb->regs.eax = (uint32_t)heap_malloc(&pcb->heap, pcb->regs.ebx);
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_free(PCB_t *pcb) {
    _load_page_dir(pcb->regs.cr3);
    heap_free(&pcb->heap, (void *)pcb->regs.ebx);
    set_process_as_ready(pcb);
}

static void sys_call_pid(PCB_t *pcb) {
    pcb->regs.eax = pcb->pid;
    set_process_as_ready(pcb);
}

static void sys_call_ppid(PCB_t *pcb) {
    pcb->regs.eax = pcb->ppid;
    set_process_as_ready(pcb);
}

static void sys_call_exec(PCB_t *pcb) {
    char filename[256];
    strcpy(filename, (char *)pcb->regs.ebx);
    _load_page_dir(PAGE_DIR_ADDR);
    set_process_as_ready(pcb);

    pcb->regs.eax = 0;
    PCB_t *child = create_process(filename, pcb->pid, pcb->stdout, pcb->shell_id);
    if (child != NULL) {
        pcb->regs.eax = child->pid;
        set_process_as_ready(child);
    }
    last_exit_code = pcb->regs.eax;
}

static void sys_call_open(PCB_t *pcb) {
    char *filename = (char *)pcb->regs.ebx;
    if (file_exists(filename) == 0) {
        pcb->regs.eax = 1;
    } else {
        pcb->regs.eax = open_file(filename);
        if (pcb->regs.eax == 0) {
            char *open_file = (char *) kmalloc(FILE_NAME_LEN);
            strcpy(open_file, filename);
            list_add_last(pcb->open_files, open_file);
        }
    }
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static uint8_t filename_cmp(void *f1, void *f2) {
    return strcmp((char *)f1, (char *)f2) == 0;
}

static void remove_filename(void *data) {
    kfree(data);
}

static void sys_call_close(PCB_t *pcb) {
    char *filename = (char *)pcb->regs.ebx;
    if (list_contains(pcb->open_files, filename, &filename_cmp) == 1) {
        pcb->regs.eax = close_file(filename);
        list_remove_data(pcb->open_files, filename, &remove_filename);
    } else {
        pcb->regs.eax = 1;
    }
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_read(PCB_t *pcb) {
    char *filename = (char *)pcb->regs.esi;
    char *buffer = (char *)pcb->regs.ebx;
    uint32_t offset = pcb->regs.ecx;
    uint32_t len = pcb->regs.edx;

    if (file_exists(filename) == 0 || list_contains(pcb->open_files, filename, &filename_cmp) == 0) {
        pcb->regs.eax = 1;
    } else {
        pcb->regs.eax = read(filename, buffer, offset, len);
    }
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_write(PCB_t *pcb) {
    char *filename = (char *)pcb->regs.esi;
    char *buffer = (char *)pcb->regs.ebx;
    uint32_t offset = pcb->regs.ecx;
    uint32_t len = pcb->regs.edx;

    if (file_exists(filename) == 0 || list_contains(pcb->open_files, filename, &filename_cmp) == 0) {
        pcb->regs.eax = 1;
    } else {
        pcb->regs.eax = write(filename, buffer, offset, len);
    }
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_touch(PCB_t *pcb) {
    pcb->regs.eax = touch((char *)pcb->regs.ebx);
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_ls(PCB_t *pcb) {
    ls();
    set_process_as_ready(pcb);
}

static void sys_call_cat(PCB_t *pcb) {
    pcb->regs.eax = cat((char *)pcb->regs.ebx);
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_cp(PCB_t *pcb) {
    pcb->regs.eax = cp((char *)pcb->regs.ebx, (char *)pcb->regs.ecx);
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_rm(PCB_t *pcb) {
    pcb->regs.eax = rm((char *)pcb->regs.ebx);
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_ps(PCB_t *pcb) {
    print_all_processes();
    set_process_as_ready(pcb);
}

static void sys_call_lp(PCB_t *pcb) {
    print_all_programs();
    set_process_as_ready(pcb);
}

static void sys_call_clear(PCB_t *pcb) {
    clear_screen();
    set_process_as_ready(pcb);
}

static void sys_call_exit_code(PCB_t *pcb) {
    int mem_sys_call = pcb->regs.eax;
    pcb->regs.eax = last_exit_code;
    if(mem_sys_call != SYSCALL_PRINTF){
        last_exit_code = 0;                     //Only one call is permitted, per exit code
    }
    set_process_as_ready(pcb);
}

static void sys_call_wait(PCB_t *pcb) {
    if (exists_process(pcb->regs.ebx) == 0) {
        pcb->regs.eax = 1;
        last_exit_code = pcb->regs.eax;
        set_process_as_ready(pcb);
    } else {
        block_process_on_another_process(pcb);
    }
}

static void sys_call_read_line(PCB_t *pcb) {
    block_process_on_keyboard(pcb);
}

static void sys_call_fork(PCB_t *parent) {
    _load_page_dir(PAGE_DIR_ADDR);
    PCB_t *child = create_process(parent->name, parent->pid, parent->stdout, parent->shell_id);

    uint32_t i, j;
    char *buff = (char *)kmalloc(FRAME_SIZE);

    // copy registers
    child->regs.ecx = parent->regs.ecx;
    child->regs.edx = parent->regs.edx;
    child->regs.ebx = parent->regs.ebx;
    child->regs.esp = parent->regs.esp;
    child->regs.ebp = parent->regs.ebp;
    child->regs.esi = parent->regs.esi;
    child->regs.edi = parent->regs.edi;
    child->regs.eflags = parent->regs.eflags;
    child->regs.eip = parent->regs.eip;

    // copy stack
    uint32_t stack_addr = PAGE_TABLE_ADDR(PROCESS_STACK_PAGE_TABLE);
    for (i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        _load_page_dir(parent->regs.cr3);
        memcpy(buff, (char *)(stack_addr + (i * FRAME_SIZE)), FRAME_SIZE);
        _load_page_dir(child->regs.cr3);
        memcpy((char *)(stack_addr + (i * FRAME_SIZE)), buff, FRAME_SIZE);
    }

    // copy heap
    uint32_t heap_addr;
    for (i = PROCESS_HEAP_START_PAGE_TABLE; i <= PROCESS_HEAP_END_PAGE_TABLE; i++) {
        heap_addr = PAGE_TABLE_ADDR(i);
        for (j = 0; j < PAGE_TABLE_ENTRIES; j++) {
            _load_page_dir(parent->regs.cr3);
            memcpy(buff, (char *)(heap_addr + (j * FRAME_SIZE)), FRAME_SIZE);
            _load_page_dir(child->regs.cr3);
            memcpy((char *)(heap_addr + (j * FRAME_SIZE)), buff, FRAME_SIZE);
        }
    }

    kfree(buff);
    parent->regs.eax = 1; // you're the parent
    child->regs.eax = 0;  // you're the child

    last_exit_code = parent->regs.eax;
    set_process_as_ready(parent);
    set_process_as_ready(child);
}

static void sys_call_file_append(PCB_t *pcb) {
    char *filename = (char *)pcb->regs.ebx;
    char *buffer = (char *)pcb->regs.ecx;
    uint32_t file_size = get_file_size(filename);
    uint32_t buff_size = strlen(buffer);
    if (file_exists(filename) == 0)
        touch(filename);
    pcb->regs.eax = write(filename, buffer, file_size, buff_size);
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_color_command(PCB_t *pcb) {
    if (pcb->shell_id == get_focused_terminal()){
        uint32_t foreground = pcb->regs.ebx;
        uint32_t background = pcb->regs.ecx;
        set_color((uint8_t)foreground | (uint8_t)background);
        pcb->regs.eax = 0;
    }
    else {
        pcb->regs.eax = 0;
    }
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

static void sys_call_set_cursor(PCB_t *pcb) {
    if (pcb->shell_id == get_focused_terminal()){
        uint32_t xAxis = pcb->regs.ebx;
        uint32_t yAxis = pcb->regs.ecx;
        if(xAxis >= MAX_COLS || yAxis >= MAX_ROWS){
            pcb->regs.eax = 1;
        }
        else{
            set_cursor(get_offset(xAxis, yAxis));
            pcb->regs.eax = 0;
        }
    }
    else {
        pcb->regs.eax = 0;
    }
    last_exit_code = pcb->regs.eax;
    set_process_as_ready(pcb);
}

void sys_callback() {
    PCB_t *pcb = get_running_process();
    switch (pcb->regs.eax) {
        case SYSCALL_EXIT:
            sys_call_exit(pcb);
            break;
        case SYSCALL_PRINTF:
            sys_call_printf(pcb);
            break;
        case SYSCALL_READ_LINE:
            sys_call_read_line(pcb);
            break;
        case SYSCALL_MALLOC:
            sys_call_malloc(pcb);
            break;
        case SYSCALL_FREE:
            sys_call_free(pcb);
            break;
        case SYSCALL_PID:
            sys_call_pid(pcb);
            break;
        case SYSCALL_PPID:
            sys_call_ppid(pcb);
            break;
        case SYSCALL_EXEC:
            sys_call_exec(pcb);
            break;
        case SYSCALL_OPEN:
            sys_call_open(pcb);
            break;
        case SYSCALL_CLOSE:
            sys_call_close(pcb);
            break;
        case SYSCALL_READ:
            sys_call_read(pcb);
            break;
        case SYSCALL_WRITE:
            sys_call_write(pcb);
            break;
        case SYSCALL_TOUCH:
            sys_call_touch(pcb);
            break;
        case SYSCALL_LS:
            sys_call_ls(pcb);
            break;
        case SYSCALL_CAT:
            sys_call_cat(pcb);
            break;
        case SYSCALL_CP:
            sys_call_cp(pcb);
            break;
        case SYSCALL_RM:
            sys_call_rm(pcb);
            break;
        case SYSCALL_PS:
            sys_call_ps(pcb);
            break;
        case SYSCALL_LP:
            sys_call_lp(pcb);
            break;
        case SYSCALL_FORK:
            sys_call_fork(pcb);
            break;
        case SYSCALL_WAIT:
            sys_call_wait(pcb);
            break;
        case SYSCALL_CLEAR:
            sys_call_clear(pcb);
            break;
        case SYSCALL_EXIT_CODE:
            sys_call_exit_code(pcb);
            break;
        case FILE_APPEND:
            sys_call_file_append(pcb);
            break;
        case SYSCALL_COLOR:
            sys_call_color_command(pcb);
            break;
        case SYSCALL_SET_CURSOR:
            sys_call_set_cursor(pcb);
            break;
        default:
            set_color(FOREGROUND_LIGHTRED);
            kprintf("ERR: Unknown system call %d\n\r", pcb->regs.eax);
            reset_color();
            kill_process(pcb);
            break;
    }
}

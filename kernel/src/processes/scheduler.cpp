#include <common.h>
#include <processes/list.h>
#include <processes/scheduler.h>
#include <drivers/screen/screen.h>
#include <string.h>
#include <fs/vfs.h>
#include <memory.h>

extern "C" {
    void _switch_task(regs_t *regs);
}

static PCB_t *running_process;
static PCB_t *latest_running_non_idle_process[NUMBER_OF_TERMINALS];
static PCB_t *idle_process;
static uint32_t focused_terminal;

list_t *ready_processes = NULL;
list_t *all_processes = NULL;
list_t *blocked_on_process_processes = NULL;
list_t *blocked_on_keyboard_processes = NULL;

uint8_t is_scheduler_initialized() {
    return all_processes != NULL;
}

PCB_t *get_running_process() {
    return running_process;
}

PCB_t *get_latest_running_non_idle_process() {
    return latest_running_non_idle_process[focused_terminal-1];
}

void print_all_processes() {
    list_print(all_processes, &print_pcb);
}

static uint8_t comparePcb(void *data1, void *data2){
    return data1 == data2;
}

uint8_t is_blocked_elsewhere(PCB_t *pcb, list_t *originalQueue) {
    if(blocked_on_keyboard_processes != originalQueue && list_contains(blocked_on_keyboard_processes, pcb, comparePcb)){
        return 1;
    }
    if(blocked_on_process_processes != originalQueue && list_contains(blocked_on_process_processes, pcb, comparePcb)){
        return 1;
    }
    return 0;
}

void switch_to_next_process() {
    if(all_processes->first == all_processes->last){
        set_color(FOREGROUND_GREEN);
        kprintf("System shutting down ...\r\n");
        kprintf("(It is safe to turn off the PC now)\r\n");
        _panic();
    }
    running_process = idle_process;
    while (ready_processes->first != NULL && running_process == idle_process) {
        running_process = (PCB_t *)ready_processes->first->data;
        list_remove(ready_processes, 0, NULL);
    }
    if (running_process != idle_process) {
        latest_running_non_idle_process[running_process->shell_id-1] = running_process;
    }
    switch_process(running_process);
}

void switch_process(PCB_t *pcb) {
    pcb->state = PROCESS_STATE_RUNNING;
    _load_page_dir(pcb->regs.cr3);
    _switch_task(&pcb->regs);
}

void set_process_as_ready(PCB_t *pcb) {
    if (list_contains(ready_processes, pcb, comparePcb)) {
        return;
    }
    pcb->state = PROCESS_STATE_READY;
    list_add_last(ready_processes, pcb);
}

void block_process_on_another_process(PCB_t *pcb) {
    pcb->state = PROCESS_STATE_WAITING;
    list_add_last(blocked_on_process_processes, pcb);
}

void wake_up_parent_process(uint32_t ppid, uint32_t exit_code) {
    PCB_t *parent = NULL;
    list_node_t *curr = blocked_on_process_processes->first;
    for (; curr != NULL; curr = curr->next) {
        if (((PCB_t *)curr->data)->pid == ppid) {
            parent = (PCB_t *)curr->data;
            break;
        }
    }
    if (parent == NULL)
        return;

    list_remove_data(blocked_on_process_processes, parent, NULL);
    parent->regs.eax = exit_code;
    if(is_blocked_elsewhere(parent, blocked_on_process_processes) == 0){
        set_process_as_ready(parent);
    }
}

uint8_t compare_by_pid(void *data1, void *data2) {
    PCB_t *pcb = (PCB_t *)data1;
    uint32_t pid = *(uint32_t *)data2;
    return pcb->pid == pid;
}

uint8_t exists_process(uint32_t pid) {
    uint32_t *pid_storage = (uint32_t *)kmalloc(sizeof(uint32_t));
    *pid_storage = pid;
    uint8_t exists = list_contains(all_processes, pid_storage, compare_by_pid);
    kfree(pid_storage);
    return exists;
}

void wake_process_waiting_for_keyboard(char *data) {
    PCB_t *pcb = (PCB_t *)list_get(blocked_on_keyboard_processes, 0);
    if (pcb == NULL)
        return;

    if (pcb->shell_id != focused_terminal){
        return;
    }

    uint32_t cr3 = _get_page_dir();
    _load_page_dir(pcb->regs.cr3);

    strcpy((char *)pcb->regs.edi, data);
    print_to_stream(pcb, data, 1);      // 1  Adds newline

    list_remove(blocked_on_keyboard_processes, 0, NULL);
    if(is_blocked_elsewhere(pcb, blocked_on_keyboard_processes) == 0){
        set_process_as_ready(pcb);
    }
    _load_page_dir(cr3);
}

void block_process_on_keyboard(PCB_t *pcb) {
    pcb->state = PROCESS_STATE_WAITING;
    list_add_first(blocked_on_keyboard_processes, pcb);
}

uint32_t get_focused_terminal() {
    return focused_terminal;
}

static void reschedule_process(uint32_t pid, list_t *blocked_processes) {
    list_t *pcbs = list_create();
    list_node_t *curr = blocked_processes->first;
    while (curr != NULL) {
        if (((PCB_t *)curr->data)->shell_id == pid)
            list_add_last(pcbs, curr->data);
        curr = curr->next;
    }
    curr = pcbs->first;
    while (curr != NULL) {
        list_remove_data(blocked_processes, curr->data, NULL);
        curr = curr->next;
    }
    curr = pcbs->last;
    while (curr != NULL) {
        list_add_first(blocked_processes, curr->data);
        curr = curr->prev;
    }
    list_free(&pcbs, NULL);
}

void switch_to_terminal(uint32_t pid) {
    PCB_t *pcb = NULL;
    list_node_t *curr = all_processes->first;
    while (curr != NULL) {
        if (((PCB_t *)curr->data)->pid == pid) {
            pcb = (PCB_t *)curr->data;
            break;
        }
        curr = curr->next;
    }
    if (pcb == NULL)
        return;

    focused_terminal = pid;

    reschedule_process(pid, blocked_on_keyboard_processes);
    reschedule_process(pid, blocked_on_process_processes);

    clear_screen();
    uint32_t file_size = get_file_size(pcb->stdout);
    char *buffer = (char *)kmalloc(file_size + 1);
    uint32_t offset;
    uint32_t len;
    offset = 0;
    len = file_size;
    memset(buffer, 0, file_size + 1);
    read(pcb->stdout, buffer, offset, len);
    buffer[len] = '\0';
    //kprintf("%s", buffer);
    print_string(buffer);
    kfree(buffer);

    set_color(FOREGROUND_CYAN);
    print_terminal_index(pid);
    reset_color();
}

void init_process_scheduler() {
    ready_processes = list_create();
    all_processes = list_create();
    blocked_on_process_processes = list_create();
    blocked_on_keyboard_processes = list_create();

    idle_process = create_process("idle.exe", 0, NULL, 0);
    idle_process->state = PROCESS_STATE_WAITING;
    running_process = idle_process;

    char stdout[] = "shell_?";
    int index_pos = strlen(stdout) - 1;
    int i;
    PCB_t *first = NULL;
    for (i = 0; i < NUMBER_OF_TERMINALS; i++) {
        stdout[index_pos] = '0' + (i + 1);
        touch(stdout);
        set_as_system_file(stdout);
        PCB_t *pcb = create_process("shell.exe", 0, stdout, i + 1);
        latest_running_non_idle_process[i] = pcb;
        if(i == 0) {
            first = pcb;
        }
        else {
            set_process_as_ready(pcb);
        }
    }
    set_process_as_ready(first);
    focused_terminal = 1;
    set_color(FOREGROUND_CYAN);
    print_terminal_index(1);
    reset_color();
}

void save_process_context(PCB_t *pcb, Interrupt_generic_registers_t *regs) {
    pcb->regs.eax = regs->eax;
    pcb->regs.ebx = regs->ebx;
    pcb->regs.ecx = regs->ecx;
    pcb->regs.edx = regs->edx;
    pcb->regs.esi = regs->esi;
    pcb->regs.edi = regs->edi;
    pcb->regs.esp = regs->useresp;
    pcb->regs.ebp = regs->ebp;
    pcb->regs.eflags = regs->eflags;
    pcb->regs.eip = regs->eip;
}

PCB_t *create_process(const char *filename, uint32_t ppid, const char *stdout, uint32_t shell_id) {
    PCB_t *pcb = create_process_virtual_addr_space(filename, ppid, stdout, shell_id);
    if (pcb != NULL) {
        list_add_last(all_processes, pcb);
    }
    return pcb;
}

void kill_process(PCB_t *pcb) {
    pcb->state = PROCESS_STATE_TERMINATION;
    free_pid(pcb->pid);
    unmap_process(pcb);

    // remove the pcb from all queues
    list_remove_data(ready_processes, pcb, NULL);
    list_remove_data(all_processes, pcb, NULL);
    list_remove_data(blocked_on_process_processes, pcb, NULL);
    list_remove_data(blocked_on_keyboard_processes, pcb, NULL);

    // close up all open files
    while (pcb->open_files->size != 0) {
        void *filename = list_get(pcb->open_files, 0);
        close_file((char *)filename);
        kfree(filename);
        list_remove(pcb->open_files, 0, NULL);
    }
    // we do not need to provide any remove function as we
    // just erased all filenames manually one by one
    list_free(&pcb->open_files, NULL);

    latest_running_non_idle_process[pcb->shell_id - 1] = idle_process;

    // free the pcb record
    kfree(pcb);
}
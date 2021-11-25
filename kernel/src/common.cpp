#include <common.h>
#include <fs/vfs.h>
#include <string.h>
#include <processes/scheduler.h>
#include <drivers/screen/screen.h>
#include <memory.h>

void print_to_stream(char *buffer) {
    PCB_t *pcb = get_running_process();
    print_to_stream(pcb, buffer, 0);
}

void print_to_stream(char *buffer, uint8_t addNewLine) {
    PCB_t *pcb = get_running_process();
    print_to_stream(pcb, buffer, addNewLine);
}

void print_to_stream(PCB_t *pcb, char *buffer) {
    print_to_stream(pcb, buffer, 0);
}

void print_to_stream(PCB_t *pcb, char *buffer, uint8_t addNewline) {
    if(pcb == NULL || buffer == NULL){
        return;
    }
    uint32_t buffer_len = strlen(buffer);
    uint32_t file_size = get_file_size(pcb->stdout);

    write(pcb->stdout, buffer, file_size, buffer_len);
    if(addNewline > 0){
        write(pcb->stdout, "\r\n", file_size+buffer_len, 2);
    }
    file_size = get_file_size(pcb->stdout);

    if (file_size >= MAX_SHELL_FILE_SIZE_LIMIT) {
        char *copy_buffer = (char *)kmalloc(MAX_SHELL_FILE_SIZE);
        memset(copy_buffer, 0, MAX_SHELL_FILE_SIZE);
        read(pcb->stdout, copy_buffer, file_size - MAX_SHELL_FILE_SIZE, MAX_SHELL_FILE_SIZE);
        delete_system_file(pcb->stdout);
        touch(pcb->stdout);
        set_as_system_file(pcb->stdout);
        write(pcb->stdout, copy_buffer, 0, MAX_SHELL_FILE_SIZE);
        kfree(copy_buffer);
    }
}

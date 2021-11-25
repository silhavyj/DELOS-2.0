#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <processes/process.h>
#include <interrupts/handlers.h>

#define TICKS_FOR_TASK_SWITCH 10
#define NUMBER_OF_TERMINALS    4
#define MAX_SHELL_FILE_SIZE  200

PCB_t *get_running_process();
PCB_t *get_latest_running_non_idle_process();
PCB_t *create_process(const char *filename, uint32_t ppid, const char *stdout, uint32_t shell_id);
void switch_to_next_process();
void switch_process(PCB_t *pcb);
void save_process_context(PCB_t *pcb, Interrupt_generic_registers_t *regs);
void init_process_scheduler();
void set_process_as_ready(PCB_t *pcb);
void kill_process(PCB_t *pcb);
void print_all_processes();
void block_process_on_another_process(PCB_t *pcb);
void wake_up_parent_process(uint32_t ppid, uint32_t exit_code);
uint8_t exists_process(uint32_t pid);
void block_process_on_keyboard(PCB_t *pcb);
void wake_process_waiting_for_keyboard(char *data);
uint8_t is_blocked_elsewhere(PCB_t *pcb, list_t *originalQueue);
void switch_to_terminal(uint32_t pid);
uint32_t get_focused_terminal();

#endif
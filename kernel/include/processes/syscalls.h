#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#define SYSCALL_EXIT         100
#define SYSCALL_PRINTF       101
#define SYSCALL_READ_LINE    102
#define SYSCALL_MALLOC       103
#define SYSCALL_FREE         104
#define SYSCALL_PID          105
#define SYSCALL_PPID         106
#define SYSCALL_EXEC         107
#define SYSCALL_OPEN         109
#define SYSCALL_CLOSE        110
#define SYSCALL_READ         111
#define SYSCALL_WRITE        112
#define SYSCALL_TOUCH        113
#define SYSCALL_LS           114
#define SYSCALL_CAT          115
#define SYSCALL_CP           116
#define SYSCALL_RM           117
#define SYSCALL_PS           118
#define SYSCALL_LP           119
#define SYSCALL_FORK         120
#define SYSCALL_WAIT         121
#define SYSCALL_CLEAR        122
#define SYSCALL_EXIT_CODE    123
#define FILE_APPEND          124
#define SYSCALL_COLOR        125
#define SYSCALL_SET_CURSOR   126

void sys_callback();

#endif

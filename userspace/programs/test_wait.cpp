#include <system.h>

int main() {
    const char *start = "START\n\r";
    const char *end = "END\n\r";
    const char *program = "test_malloc.exe";
    int pid;

    printf(start);
    pid = exec(program);
    wait_for_child(pid);
    printf(end);
    return 0;
}
#include <system.h>
#include <string.h>

const char *MOVE = "%c -> %c\n\r";

void hanoi(char from, char to, char tmp, int n) {
    if (n == 1) {
        printf(MOVE, from, to);
        return;
    }
    hanoi(from, tmp, to, n - 1);
    printf(MOVE, from, to);
    hanoi(tmp, to, from, n - 1);
}

int main() {
    const char *PROMPT = "disks: ";
    char buff[32];

    printf(PROMPT);
    read_line(buff);
    int disks = atoi(buff);
    hanoi('A', 'B', 'C', disks);
    return 0;
}
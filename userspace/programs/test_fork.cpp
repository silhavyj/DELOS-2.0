#include <system.h>

int main() {
    const char *msg = "Hello\n\r";
    int id = fork();
    if (id == 0) {
        printf(msg);
        if (fork() || fork())
            fork();
        printf(msg);
    } else {
        printf(msg);
    }
    return 0;
}
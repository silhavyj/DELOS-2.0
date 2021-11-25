#include <system.h>

int main() {
    const char *PROGRAM_A = "yell_A.exe";
    const char *PROGRAM_B = "yell_B.exe";

    exec(PROGRAM_A);
    exec(PROGRAM_B);

    return 0;
}
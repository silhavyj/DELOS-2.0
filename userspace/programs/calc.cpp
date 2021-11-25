#include <system.h>
#include <string.h>

int main() {
    const char *PLUS = "%d + %d = %d\n\r";
    const char *MINUS = "%d - %d = %d\n\r";
    const char *MULT = "%d * %d = %d\n\r";
    const char *TERMINATE_PROGRAM = "Do you want to exit the program? [1/0]: ";
    const char *ENTER_X = "Enter x: ";
    const char *ENTER_Y = "Enter y: ";

    int x, y;
    int end = 0;
    char buffer[32];

    while (end == 0) {
        printf(ENTER_X);
        read_line(buffer);
        x = atoi(buffer);

        printf(ENTER_Y);
        read_line(buffer);
        y = atoi(buffer);

        printf(PLUS, x, y, x + y);
        printf(MINUS, x, y, x - y);
        printf(MULT, x, y, x * y);

        printf(TERMINATE_PROGRAM);
        read_line(buffer);
        end = atoi(buffer);
    }

    return 0;
}
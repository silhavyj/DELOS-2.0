#include <system.h>
#include <string.h>
#include <memory.h>

#define BUFF_SIZE 32

int main() {
    const char *FILENAME = "fib.txt";
    const char *OPEN_ERR = "error when opening the file!\n\r";
    const char *CLOSE_ERR = "error when closing the file!\n\r";
    const char *TOUCH_ERR = "error when touching the file!\n\r";
    const char *WRITE_ERR = "error when writing into the file!\n\r";
    const char *PROMPT = "enter n: ";

    char buff[BUFF_SIZE];
    printf(PROMPT);
    read_line(&buff[0]);
    int n = atoi(&buff[0]);
    int offset = 0;
    int len;

    if (touch(FILENAME) != 0) {
        printf(TOUCH_ERR);
        return 1;
    }
    if (open(FILENAME) != 0) {
        printf(OPEN_ERR);
        return 2;
    }

    memset(&buff[0], 0, BUFF_SIZE);
    if (n >= 1) {
        buff[0] = '1';
        buff[1] = ' ';
        len = strlen(buff);
        if (write((char *)FILENAME, &buff[0], offset, len) != 0) {
            printf(WRITE_ERR);
            return 3;
        }
        offset += len;
    }
    if (n >= 2) {
        buff[0] = '1';
        buff[1] = ' ';
        len = strlen(buff);
        if (write((char *)FILENAME, &buff[0], offset, len) != 0) {
            printf(WRITE_ERR);
            return 3;
        }
        offset += len;
    }
    if (n > 2) {
        int a = 1, b = 1, c;
        for (int i = 0; i < n - 2; i++) {
            c = a + b;
            a = b;
            b = c;
            memset(&buff[0], 0, BUFF_SIZE);
            itoa_dec(buff, c);
            len = strlen(buff);
            buff[len++] = ' ';
            if (write((char *)FILENAME, &buff[0], offset, len) != 0) {
                printf(WRITE_ERR);
                return 3;
            }
            offset += len;
        }
    }

    if (close(FILENAME) != 0) {
        printf(CLOSE_ERR);
        return 4;
    }
    return 0;
}
#include <system.h>

#define N 5

int main() {
    const char *separator = "--------\n\r";
    const char *format = "0x%x %d\n\r";

    int *arr;

    arr = (int *)malloc(N *sizeof(int));
    for (int i = 0; i < N; i++) {
        arr[i] = i;
        printf(format, &arr[i], arr[i]);
    }
    free(arr);

    printf(separator);

    arr = (int *)malloc(N *sizeof(int));
    for (int i = 0; i < N; i++) {
        arr[i] = i;
        printf(format, &arr[i], arr[i]);
    }
    free(arr);
    return 0;
}
#include <math.h>

uint32_t pow(uint32_t x, uint32_t y) {
    int result = 1;
    while (y > 0) {
        if (y % 2 == 0) // y is even
        {
            x = x * x;
            y = y / 2;
        }
        else // y isn't even
        {
            result = result * x;
            y = y - 1;
        }
    }
    return result;
}
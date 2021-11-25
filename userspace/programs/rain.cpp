#include <system.h>
#include <string.h>

#define BLACK 0x00
#define GREEN_FG 0x02
#define GREENLIGHT_FG 0x0A

static uint32_t next = 1;


uint32_t rand(void) // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (uint32_t)(next / 65536) % 32768;
}

void srand(uint32_t seed)
{
    next = seed;
}

int inScreenYPosition(int yPosition, int height)
{
    if (yPosition < 0)
        return yPosition + height;
    else if (yPosition < height)
        return yPosition;
    else
        return 0;
}

void update_all_columns(int width, int height, int *y)
{
    const char *ascii_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz!#$%&()/:;<>=?@\\[]{}|";
    int x;
    char char_buff[2];
    char_buff[1] = '\0';
    // draws 3 characters in each x column each time...
    // a dark green, light green, and a space

    // y is the position on the screen
    // y[x] increments 1 each time so each loop does the same thing but down 1 y value
    for (x = 0; x < width; ++x)
    {
        // the bright green character
        color_screen_command(GREENLIGHT_FG, BLACK); // Set color to green
        set_cursor_command(x, y[x]);                // Reset cursor
        char_buff[0] = ascii_chars[rand() % strlen(ascii_chars)];
        printf(char_buff);

        color_screen_command(GREEN_FG, BLACK); // Set color to green
        set_cursor_command(x, inScreenYPosition(y[x] - 2, height));           // Reset cursor
        char_buff[0] = ascii_chars[rand() % strlen(ascii_chars)];
        printf(char_buff);

        color_screen_command(GREEN_FG, BLACK); // Set color to green
        set_cursor_command(x, inScreenYPosition(y[x] - 8, height));           // Reset cursor
        char_buff[0] = ' ';

        printf(char_buff);

        // increment y
        y[x] = inScreenYPosition(y[x] + 1, height);
    }

    for(volatile int i = 0; i < 0x8000000; i++){
        ;
    }
}

int main()
{
    int width = 80;
    int height = 25;
    int y[width];

    clear_screen_command();                // Clear screen
    color_screen_command(GREEN_FG, BLACK); // Set color to green
    set_cursor_command(0, 0);              // Reset cursor to X=0 Y=0

    for (int x = 0; x < width; ++x)
    {
        // gets random number between 0 and 24
        y[x] = rand() % height;
    }

    while (true)
    {
        update_all_columns(width, height, y);
    }

    return 0;
}
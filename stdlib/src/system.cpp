#include <system.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>

void printf(const char *str, ...) {
    char buff[PRINT_BUFF_SIZE];
    char convertor_buffer[PRINT_BUFF_SIZE];
    memset(buff, 0, PRINT_BUFF_SIZE);
    va_list lst;
    va_start(lst, str);

    while(*str != '\0')
    {
        if(*str != '%')
        {
            append(buff, *str);
            str++;
            continue;
        }

        str++;

        if(*str == '\0')
        {
            break;
        }

        switch(*str)
        {
            case 'c':
                append(buff, (char)va_arg(lst, int));
                break;
            case 's':
                strcat(buff, va_arg(lst, char*));
                break;
            case 'd':
                strcat(buff, itoa_dec(convertor_buffer, va_arg(lst, int)));
                break;
            case 'x':
                strcat(buff, itoa_hex(convertor_buffer, va_arg(lst, int)));
                break;
            case 'f':
                break;
            default:
                append(buff, '%');
                break;
        }
        str++;
    }
    va_end(lst);
    asm (
        "mov    %0, %%esi;"
        "mov    $101, %%eax;"
        "int    $0x80;" : : "g" (buff)
    );
}
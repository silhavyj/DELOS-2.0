#include <string.h>
#include <math.h>

uint32_t strlen(const char* str) {
    uint32_t i = 0;
    while(str[i] != '\0') {
        ++i;
    }
    return i;
}

void append(char* str, char c) {
    uint32_t length = strlen(str);
    str[length] = c;
    str[length + 1] = '\0';
}

uint32_t strcmp(const char* str1, const char* str2) {
    uint32_t i;
    for (i = 0; str1[i] == str2[i]; i++) {
        if (str1[i] == '\0') return 0;
    }
    return str1[i] - str2[i];
}

uint32_t strcmp(const char* str1, const char* str2, uint32_t n) {
    uint32_t i;
    for (i = 0; str1[i] == str2[i]; i++) {
        if (str1[i] == '\0' || i >= n) return 0;
    }
    return str1[i] - str2[i];
}

char* strncat(char* s1, const char* s2, uint32_t n)
{
    // Pointer should not null pointer
    if((s1 == NULL) && (s2 == NULL)) {
        return NULL;
    }
    // Create copy of s1
    char *dest = s1;
    // Find the end of the destination string
    while(*dest != '\0') {
        dest++;
    }
    // Now append the source string characters
    // Until not get null character of s2 or n != 0
    while (n--) {
        if (!(*dest++ = *s2++)) {
            return s1;
        }
    }
    // Append null character in the last
    *dest = '\0';
    return s1;
}

char* strcat(char* s1, const char* s2)
{
    // Pointer should not null pointer
    if(s1 == NULL && s2 == NULL) {
        return NULL;
    }
    // Create copy of s1
    char *start = s1;
    // Find the end of the destination string
    while(*start != '\0') {
        start++;
    }
    // Now append the source string characters
    // Until not get null character of s2
    while(*s2 != '\0') {
        *start++ = *s2++;
    }
    // Append null character in the last
    *start = '\0';
    return s1;
}

uint32_t atoi(const char* str) {
    // Variable to store the result
    uint32_t result = 0; 
    // Initialize sign as positive
    uint32_t sign = 1; 
    // If pointer is null
    if(str == NULL) {
        return 0;
    }
    //If number is negative, then update sign
    if((*str) == '-') {
        sign = -1;
        // Increment the pointer
        ++str;
    }
    // Check string validity
    while(IS_NUMERIC_STRING(str)) {
        result = (result * 10) + (*str - 48);
        //Increment the pointer
        str++;
    }
    return sign * result;
}

char* strcpy(char* dest, const char* src) {
    // Return if no memory is allocated to the destination
    if (dest == NULL) {
        return NULL;
    }
 
    // Take a pointer pointing to the beginning of the destination string
    char *ptr = dest;
 
    // Copy the C-string pointed by source into the array
    // Pointed by destination
    while ((*dest++ = *src++) != '\0');
 
    // Include the terminating null character
    *dest = '\0';

    return ptr;
}

char* strncpy(char* dest, const char* src, uint32_t n) {
    // Return if dest and src is NULL
    if (dest == NULL && src == NULL) {
        return NULL;
    }
    // Take a pointer pointing to the beginning of dest string
    char* start = dest;
    // Copy first n characters of C-string pointed by src
    // Into the array pointed by dest
    while (*src && n--) {
        *dest = *src;
        dest++;
        src++;
    }
    // Null terminate dest string
    *dest = '\0';
    return start;
}

char* reverse(char* str) {
    uint32_t start, end;
    char temp;
    uint32_t len = strlen(str);
    for(start = 0, end = len - 1; start < end; start++, end--) {
        temp = *(str + start);
        *(str + start) = *(str + end);
        *(str + end) = temp;
    }
    return str;
}

char* itoa(char* str, int32_t num, uint32_t base) {
    // Check that the base if valid
    if (base < 2 || base > 36) { 
        *str = '\0'; 
        return str; 
    }

    char* ptr = str, *ptr1 = str, tmp_char;
    int32_t tmp_value;

    do {
        tmp_value = num;
        num /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - num * base)];
    } while (num);

    // Apply negative sign
    if (tmp_value < 0) {
        *ptr++ = '-';
    }
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return str;
}

char* itoa_dec(char* str, int32_t num) {
    return itoa(str, num, 10);
}

char* itoa_hex(char* str, int32_t num) {
    return itoa(str, num, 16);
}

char* itoa_bin(char* str, int32_t num) {
    return itoa(str, num, 2);
}
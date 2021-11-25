#ifndef _STRING_H_
#define _STRING_H_

#include <stdint.h>

#define IS_NUMERIC_STRING(d) (*(char*)d >= 48) && (*(char*)d <= 57)

uint32_t strlen(const char *str);
uint32_t strcmp(const char* str1, const char* str2);
uint32_t strcmp(const char* str1, const char* str2, uint32_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, uint32_t n);
char* reverse(char* str);
void append(char* str, char c);
char* strncat(char* s1, const char* s2, uint32_t n);
char* strcat(char* s1, const char* s2);
uint32_t append(char* str1, const char* str2);
uint32_t atoi(const char* str);
uint32_t find_first_not_of(const char *str, char c);

char* itoa(char* str, int32_t num, uint32_t base);
char* itoa_dec(char* str, int32_t num);
char* itoa_hex(char* str, int32_t num);
char* itoa_bin(char* str, int32_t num);

#endif
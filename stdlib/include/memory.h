#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <stdint.h>

void* memcpy(void* dest, const void* src, uint32_t size);
void* memset(void* dest, unsigned char value, uint32_t size);
int memcmp(const void* p1, const void* p2, uint32_t size);
void* memmove(void* dest, const void* src, uint32_t size);

#endif
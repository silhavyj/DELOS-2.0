#include <memory.h>

void* memcpy(void* dest, const void* src, uint32_t size) {
    char* destCopy = (char*)dest;
    const char* srcCopy = (const char*)src;
    if((destCopy != NULL) && (srcCopy != NULL)) {
        while(size) {
            *(destCopy++)= *(srcCopy++);
            --size;
        }
    }
    return dest;
}

void* memset(void* dest, unsigned char value, uint32_t size) {    
    char* destCopy = (char*)dest;
    uint32_t i;
    for (i = 0; i < size; i++) {
        destCopy[i] = value;
    }
    return dest;
}

int memcmp(const void* p1, const void* p2, uint32_t size) {
    unsigned char* p = (unsigned char*)p1;
    unsigned char* q = (unsigned char*)p2;
    int charCompareStatus = 0;
    //If both pointer pointing same memory block
    if (p1 == p2)
    {
        return charCompareStatus;
    }
    while (size > 0)
    {
        if (*p != *q)
        {
            //compare the mismatching character
            charCompareStatus = (*p >*q)?1:-1;
            break;
        }
        size--;
        p++;
        q++;
    }
    return charCompareStatus;
}

// Source: https://aticleworld.com/memmove-function-implementation-in-c/
void* memmove(void* dest, const void* src, uint32_t size) {
    char* srcCopy = (char*)src;
    char* destCopy = (char*)dest;
    if (srcCopy == NULL && destCopy == NULL) {
        return NULL;
    }
    if((srcCopy < destCopy) && (destCopy < srcCopy + size)) {
        for (destCopy += size, srcCopy += size; size--;) {
            *--destCopy = *--srcCopy;
        }
    }
    else {
        while(size--) {
            *destCopy++ = *srcCopy++;
        }
    }
    return dest;
}
#ifndef _HEAP_H_
#define _HEAP_H_

#include <stdint.h>

typedef struct heap_block {
    struct heap_block *next;    // next block of memory within the heap
    uint32_t free : 1;          // flag if the block is free
    uint32_t size;              // size of the block in bytes
} __attribute__((packed)) heap_block_t;

typedef struct {
    uint32_t addr;              // start addr of the heap
    uint32_t size;              // size of the heap
} __attribute__((packed)) heap_t;


uint32_t get_kernel_heap_size();
void heap_init(heap_t *heap, uint32_t addr, uint32_t size);
void *heap_malloc(heap_t *heap, uint32_t size);
void heap_free(heap_t *heap, void *ptr);

int kernel_heap_init();
void *kmalloc(uint32_t size);
void kfree(void *ptr);

#endif
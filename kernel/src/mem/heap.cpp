#include <mem/heap.h>
#include <mem/paging.h>

static heap_t kernel_heap;

void heap_init(heap_t *heap, uint32_t addr, uint32_t size) {
    heap->addr = addr;
    heap->size = size;

    // init the very first block which is free and takes
    // up the entire size of the heap; once malloc() is called
    // it will be broken down into two pieces depending on the size
    // passed into the malloc function
    heap_block_t *block = reinterpret_cast<heap_block_t *>(addr);
    block->size = size;
    block->next = NULL;
    block->free = 1;
}

uint32_t get_kernel_heap_size() {
    uint32_t size = 0;
    heap_block_t *block = reinterpret_cast<heap_block_t *>(kernel_heap.addr);
    while (block != NULL) {
        if(block->free == 1){
            size += block->size;
        }
        block = block->next;
    }
    return size;
}

void *heap_malloc(heap_t *heap, uint32_t size) {
    heap_block_t *block = reinterpret_cast<heap_block_t *>(heap->addr);

    // we need to store the header of the block as well
    uint32_t actual_size_needed = size + sizeof(heap_block_t);

    // iterate through the chain of blocks until
    // you find one that has a suitable size
    while (block != NULL) {

        // if two adjacent blocks are free, merge them into one larger block
        if (block->free == 1 && block->next != NULL && block->next->free == 1) {
            block->size += block->next->size;
            block->next = block->next->next;
        }
        // check if the block is big enough
        if (block->free == 1 && block->size >= actual_size_needed) {
            uint32_t block_addr = reinterpret_cast<uint32_t>(block);

            // check if there's enough room for another block
            // after we break this one apart (there must be heap_block_t
            // and at least one byte after that, hence '>' and not '>=')
            if (block->size > actual_size_needed + sizeof(heap_block_t)) {
                heap_block_t *new_block = reinterpret_cast<heap_block_t *>(block_addr + actual_size_needed);
                new_block->free = 1;
                new_block->next = block->next;
                new_block->size = block->size - actual_size_needed;

                block->next = new_block;
                block->size = actual_size_needed;
            }
            block->free = 0;
            return reinterpret_cast<void *>(block_addr + sizeof(heap_block_t));
        }
        // move on to the next block
        block = block->next;
    }
    return NULL; // no memory left :(
}

void heap_free(heap_t *heap, void *ptr) {
    uint32_t addr = reinterpret_cast<uint32_t>(ptr);
    // check if the address belongs to the heap space
    if (addr < heap->addr || addr > (heap->addr + heap->size)) {
        return;
    }
    // mark the block as free
    heap_block_t *block = reinterpret_cast<heap_block_t *>(addr - sizeof(heap_block_t));
    block->free = 1;
}

int kernel_heap_init() {
    heap_init(&kernel_heap, KERNEL_HEAP_START_ADDR, KERNEL_HEAP_SIZE);
    return 0;
}

void *kmalloc(uint32_t size) {
    return heap_malloc(&kernel_heap, size);
}

void kfree(void *ptr) {
    heap_free(&kernel_heap, ptr);
}
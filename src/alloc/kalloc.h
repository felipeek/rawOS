#ifndef RAW_OS_ALLOC_KALLOC_H
#define RAW_OS_ALLOC_KALLOC_H
#include "../common.h"
// Wrapper over kalloc_heap
#define KERNEL_HEAP_ADDRESS 0x00500000

void kalloc_init(u32 initial_pages);
void* kalloc_alloc(u32 size);
void kalloc_free(void* ptr);
void* kalloc_realloc(void* ptr, u32 old_size, u32 new_size);
void kalloc_print();
#endif
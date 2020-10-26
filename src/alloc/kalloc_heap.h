#ifndef RAW_OS_ALLOC_KALLOC_HEAP_H
#define RAW_OS_ALLOC_KALLOC_HEAP_H
#include "kalloc_avl.h"
typedef struct {
	u32 avl_initial_addr;
	u32 initial_addr;
	u32 size;
	Kalloc_AVL avl;
} Kalloc_Heap;

void kalloc_heap_create(Kalloc_Heap* heap, u32 initial_addr, u32 initial_pages);
void* kalloc_heap_alloc(Kalloc_Heap* heap, u32 size, u32 alignment);
void kalloc_heap_free(Kalloc_Heap* heap, void* ptr);
void kalloc_heap_print(const Kalloc_Heap* heap);
#endif
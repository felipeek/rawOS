#include "kalloc.h"
#include "kalloc_heap.h"

Kalloc_Heap heap;

void kalloc_init(u32 initial_pages) {
	kalloc_heap_create(&heap, KERNEL_HEAP_ADDRESS, initial_pages);
}

void* kalloc_alloc(u32 size) {
	return kalloc_heap_alloc(&heap, size);
}

void kalloc_free(void* ptr) {
	kalloc_heap_free(&heap, ptr);
}
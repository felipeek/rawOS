#include "kalloc.h"
#include "kalloc_heap.h"
#include "../util/util.h"

Kalloc_Heap heap;

void kalloc_init(u32 initial_pages) {
	kalloc_heap_create(&heap, KERNEL_HEAP_ADDRESS, initial_pages);
	//kalloc_test(&heap); while(1);
}

void* kalloc_alloc(u32 size) {
	return kalloc_heap_alloc(&heap, size, 0x0);
}

void kalloc_free(void* ptr) {
	kalloc_heap_free(&heap, ptr);
}

void* kalloc_alloc_aligned(u32 size, u32 alignment) {
	return kalloc_heap_alloc(&heap, size, alignment);
}

void* kalloc_realloc(void* ptr, u32 old_size, u32 new_size) {
	if (new_size <= old_size) {
		return ptr;
	}
	void* mem = kalloc_heap_alloc(&heap, new_size, 0x0);
	util_memcpy(mem, ptr, old_size);
	kalloc_free(ptr);
	return mem;
}

void kalloc_print() {
	kalloc_print(&heap);
}
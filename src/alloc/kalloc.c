#include "kalloc.h"
#include "kalloc_heap.h"
#include "../util/util.h"

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

void* kalloc_realloc(void* ptr, u32 old_size, u32 new_size) {
	if (new_size <= old_size) {
		return ptr;
	}
	void* mem = kalloc_heap_alloc(&heap, new_size);
	util_memcpy(mem, ptr, old_size);
	kalloc_free(ptr);
	return mem;
}

void kalloc_print() {
	kalloc_print(&heap);
}
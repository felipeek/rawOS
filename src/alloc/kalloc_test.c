#include "kalloc_test.h"
#include "../common.h"
#include "../paging.h"
#include "../util/util.h"
#include "../screen.h"

#define HEAP_HEADER_MAGIC 0xABCD
#define HEAP_FOOTER_MAGIC 0xEF01

typedef struct {
	u16 magic;
	u32 size;
	s32 used;
} Kalloc_Heap_Header;

typedef struct {
	u16 magic;
	Kalloc_Heap_Header* header;
} Kalloc_Heap_Footer;

typedef struct {
	u32 size;
	void* ptr;
} Allocd_Data;

Allocd_Data* alloc_datas;
u32 alloc_datas_size = 0;

u32 rand_aux = 132145;
s32 rand() {
	rand_aux += 132145;
	return (s32)(((rand_aux + 21404213) * 23469) % 32767);
}

static void check_heap(const Kalloc_Heap* heap) {
	// Check overall heap structure
	Kalloc_Heap_Header* header = (Kalloc_Heap_Header*)heap->initial_addr;
	while ((u8*)header < ((u8*)heap->initial_addr + heap->size)) {
		Kalloc_Heap_Footer* footer = (Kalloc_Heap_Footer*)((u8*)header + sizeof(Kalloc_Heap_Header) + header->size);
		util_assert("footer->header == header", footer->header == header);
		util_assert("footer->magic == HEAP_FOOTER_MAGIC", footer->magic == HEAP_FOOTER_MAGIC);
		util_assert("header->magic == HEAP_HEADER_MAGIC", header->magic == HEAP_HEADER_MAGIC);
		util_assert("header->size < 1000000", header->size < 1000000);
		util_assert("header->used == 0 || header->used == 1", header->used == 0 || header->used == 1);
		header = (Kalloc_Heap_Header*)((u8*)header + sizeof(Kalloc_Heap_Header) + header->size + sizeof(Kalloc_Heap_Footer));
	}

	// Check that we are not receiving same addresses
	for (u32 i = 0; i < alloc_datas_size; ++i) {
		Allocd_Data current = alloc_datas[i];
		for (u32 j = i + 1; j < alloc_datas_size; ++j) {
			util_assert("current.ptr != alloc_datas[j].ptr", current.ptr != alloc_datas[j].ptr);
		}
	}

	// Check that the heap headers are correct.
	for (u32 i = 0; i < alloc_datas_size; ++i) {
		Allocd_Data current = alloc_datas[i];
		Kalloc_Heap_Header* header = (Kalloc_Heap_Header*)((u8*)current.ptr - sizeof(Kalloc_Heap_Header));
		util_assert("heap header not correct", header->size >= current.size && header->size <= current.size + sizeof(Kalloc_Heap_Header) + sizeof(Kalloc_Heap_Footer));
	}
}

static void alloc_data(Kalloc_Heap* heap, u32 size) {
	Allocd_Data ad;
	ad.ptr = kalloc_heap_alloc(heap, size);
	ad.size = size;
	alloc_datas[alloc_datas_size++] = ad;
	check_heap(heap);
	return;
}

static void free_random_data(Kalloc_Heap* heap) {
	u32 selected_index = rand() % alloc_datas_size;
	kalloc_heap_free(heap, alloc_datas[selected_index].ptr);

	for (u32 i = selected_index; i < alloc_datas_size - 1; ++i){ 
		alloc_datas[i] = alloc_datas[i + 1];
	}

	--alloc_datas_size;
}

void kalloc_test(Kalloc_Heap* empty_heap) {
	const u32 alloc_data_addr = 0x10000000;
	for (u32 i = 0; i < 128; ++i) {
		paging_create_page_with_any_frame(alloc_data_addr / 0x1000 + i);
	}
	alloc_datas = (Allocd_Data*)alloc_data_addr;
	alloc_datas_size = 0;

	for (u32 i = 0; i < 2000; ++i) {
		u32 r = rand() % 100;
		if (r < 75 || alloc_datas_size == 0) {
			u32 size = (u32)rand() % 1024;
			alloc_data(empty_heap, size);
		} else {
			free_random_data(empty_heap);
		}
	}
	for (u32 i = 0; i < 2000; ++i) {
		u32 r = rand() % 100;
		if (r < 25 || alloc_datas_size == 0) {
			u32 size = (u32)rand() % 1024;
			alloc_data(empty_heap, size);
		} else {
			free_random_data(empty_heap);
		}
	}
	for (u32 i = 0; i < 2000; ++i) {
		u32 r = rand() % 100;
		if (r < 75 || alloc_datas_size == 0) {
			u32 size = (u32)rand() % 1024;
			alloc_data(empty_heap, size);
		} else {
			free_random_data(empty_heap);
		}
	}
	for (u32 i = 0; i < 2000; ++i) {
		u32 r = rand() % 100;
		if (r < 22 || alloc_datas_size == 0) {
			u32 size = (u32)rand() % 1024;
			alloc_data(empty_heap, size);
		} else {
			free_random_data(empty_heap);
		}
	}
	kalloc_heap_print(empty_heap);
}
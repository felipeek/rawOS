#include "kalloc_heap.h"
#include "../util/util.h"
#include "../screen.h"
#include "../paging.h"

#define HEAP_HEADER_MAGIC 0xABCD
#define HEAP_FOOTER_MAGIC 0xEF01
#define NUM_PAGES_RESERVED_FOR_AVL 16
#define HEAP_MAX_SIZE 64 * 1024 * 1024
#define PAGE_SIZE 4096

typedef struct {
	u16 magic;
	u32 size;
	s32 used;
} Kalloc_Heap_Header;

typedef struct {
	u16 magic;
	Kalloc_Heap_Header* header;
} Kalloc_Heap_Footer;

#define COMPLEX_HEAP_ENABLED

#ifdef COMPLEX_HEAP_ENABLED
void kalloc_heap_create(Kalloc_Heap* heap, u32 initial_addr, u32 initial_pages) {
	util_assert("kalloc: initial address must be page-alinged", initial_addr % 0x1000 == 0);
	util_assert("kalloc: insufficient number of initial pages", initial_pages * PAGE_SIZE >= sizeof(Kalloc_Heap_Footer) + sizeof(Kalloc_Heap_Header));

	for (int i = 0; i < NUM_PAGES_RESERVED_FOR_AVL + initial_pages; ++i) {
		paging_create_page_with_any_frame(initial_addr / PAGE_SIZE + i);
	}

	heap->avl_initial_addr = initial_addr;
	heap->initial_addr = initial_addr + NUM_PAGES_RESERVED_FOR_AVL * PAGE_SIZE;
	heap->size = initial_pages * PAGE_SIZE;

	kalloc_avl_init(&heap->avl, (void*)heap->avl_initial_addr, NUM_PAGES_RESERVED_FOR_AVL * PAGE_SIZE);

	Kalloc_Heap_Header* first_header = (Kalloc_Heap_Header*)heap->initial_addr;
	Kalloc_Heap_Footer* first_footer = (Kalloc_Heap_Footer*)((unsigned char*)heap->initial_addr + heap->size - sizeof(Kalloc_Heap_Footer));

	first_header->magic = HEAP_HEADER_MAGIC;
	first_header->size = heap->size - sizeof(Kalloc_Heap_Header) - sizeof(Kalloc_Heap_Footer);
	first_header->used = 0;
	first_footer->header = first_header;
	first_footer->magic = HEAP_FOOTER_MAGIC;
	
	kalloc_avl_insert(&heap->avl, first_header->size, first_header);
}

void* kalloc_heap_alloc(Kalloc_Heap* heap, u32 size) {
	Kalloc_Heap_Header* target_hole = kalloc_avl_find_hole(&heap->avl, size);

	if (target_hole) {
		// We found a hole
		util_assert("kalloc: found hole in inconsistent state (used == 1)", target_hole->used == 0);
		u32 hole_size = target_hole->size;

		kalloc_avl_remove(&heap->avl, target_hole->size, target_hole);
		
		// If we still have enough free space in the hole, we generate a new hole.
		if (target_hole->size > size + sizeof(Kalloc_Heap_Header) + sizeof(Kalloc_Heap_Footer)) {
			Kalloc_Heap_Header* new_block_header = target_hole;
			Kalloc_Heap_Footer* new_block_footer = (Kalloc_Heap_Footer*)((unsigned char*)new_block_header + sizeof(Kalloc_Heap_Header) + size);
			Kalloc_Heap_Header* new_hole_header = (Kalloc_Heap_Header*)((unsigned char*)new_block_footer + sizeof(Kalloc_Heap_Footer));
			Kalloc_Heap_Footer* new_hole_footer = (Kalloc_Heap_Footer*)((unsigned char*)target_hole + sizeof(Kalloc_Heap_Header) + target_hole->size);

			new_block_header->size = size;
			new_block_header->used = 1;
			new_block_footer->magic = HEAP_FOOTER_MAGIC;
			new_block_footer->header = new_block_header;
			new_hole_header->magic = HEAP_HEADER_MAGIC;
			new_hole_header->size = hole_size - size - sizeof(Kalloc_Heap_Header) - sizeof(Kalloc_Heap_Footer);
			new_hole_header->used = 0;
			new_hole_footer->header = new_hole_header;
			kalloc_avl_insert(&heap->avl, new_hole_header->size, new_hole_header);
			return (unsigned char*)new_block_header + sizeof(Kalloc_Heap_Header);
		} else {
			target_hole->used = 1;
			return (unsigned char*)target_hole + sizeof(Kalloc_Heap_Header);
		}
	} else {
		// If we were not able to find a fitting hole in the AVL, we need to expand the heap.
		screen_print("Expanding heap... Going from ");
		screen_print_u32(heap->size / PAGE_SIZE);
		screen_print(" pages to ");
		screen_print_u32(heap->size / PAGE_SIZE + 1);
		screen_print(" pages.\n");
		paging_create_page_with_any_frame((heap->initial_addr + heap->size) / PAGE_SIZE);

		Kalloc_Heap_Footer* last_footer = (Kalloc_Heap_Footer*)((unsigned char*)heap->initial_addr + heap->size - sizeof(Kalloc_Heap_Footer));
		Kalloc_Heap_Header* last_header = last_footer->header;
		heap->size += PAGE_SIZE;

		if (last_header->used) {
			Kalloc_Heap_Header* new_hole_header = (Kalloc_Heap_Header*)((unsigned char*)last_footer + sizeof(Kalloc_Heap_Footer));
			Kalloc_Heap_Footer* new_hole_footer = (Kalloc_Heap_Footer*)((unsigned char*)new_hole_header + PAGE_SIZE - sizeof(Kalloc_Heap_Footer));
			new_hole_header->magic = HEAP_HEADER_MAGIC;
			new_hole_header->size = PAGE_SIZE - sizeof(Kalloc_Heap_Header) - sizeof(Kalloc_Heap_Footer);
			new_hole_header->used = 0;
			new_hole_footer->magic = HEAP_FOOTER_MAGIC;
			new_hole_footer->header = new_hole_header;

			kalloc_avl_insert(&heap->avl, new_hole_header->size, new_hole_header);
		} else {
			Kalloc_Heap_Footer* new_footer = (Kalloc_Heap_Footer*)((unsigned char*)heap->initial_addr + heap->size - sizeof(Kalloc_Heap_Footer));
			new_footer->magic = HEAP_FOOTER_MAGIC;
			new_footer->header = last_header;

			kalloc_avl_remove(&heap->avl, last_header->size, last_header);
			last_header->size += PAGE_SIZE;
			kalloc_avl_insert(&heap->avl, last_header->size, last_header);
		}
		return kalloc_heap_alloc(heap, size);
	}
}

void kalloc_heap_free(Kalloc_Heap* heap, void* ptr) {
	Kalloc_Heap_Header* header = (Kalloc_Heap_Header*)((unsigned char*)ptr - sizeof(Kalloc_Heap_Header));
	Kalloc_Heap_Footer* footer = (Kalloc_Heap_Footer*)((unsigned char*)ptr + header->size);
	util_assert("found block in inconsistent state (used == 0)", header->used == 1);

	// Check if there is a previous header
	if ((u32)header != heap->initial_addr) {
		Kalloc_Heap_Footer* previous_footer = (Kalloc_Heap_Footer*)((unsigned char*)header - sizeof(Kalloc_Heap_Footer));
		Kalloc_Heap_Header* previous_header = previous_footer->header;

		// Check whether previous header defines a hole
		if (!previous_header->used) {
			// Remove this hole from AVL, because we will merge it with the new hole.
			kalloc_avl_remove(&heap->avl, previous_header->size, previous_header);
			previous_header->size += header->size + sizeof(Kalloc_Heap_Header) + sizeof(Kalloc_Heap_Footer);
			footer->header = previous_header;
			header = previous_header;
		}
	}

	// Check if there is a next header
	if ((u32)footer != heap->initial_addr + heap->size - sizeof(Kalloc_Heap_Footer)) {
		Kalloc_Heap_Header* next_header = (Kalloc_Heap_Header*)((unsigned char*)footer + sizeof(Kalloc_Heap_Footer));
		Kalloc_Heap_Footer* next_footer = (Kalloc_Heap_Footer*)((unsigned char*)next_header + sizeof(Kalloc_Heap_Header) + next_header->size);

		// Check whether next header defines a hole
		if (!next_header->used) {
			// Remove this hole from AVL, because we will merge it with the new hole.
			kalloc_avl_remove(&heap->avl, next_header->size, next_header);
			
			header->size += next_header->size + sizeof(Kalloc_Heap_Header) + sizeof(Kalloc_Heap_Footer);
			next_footer->header = header;
			footer = next_footer;
		}
	}

	header->used = 0;
	kalloc_avl_insert(&heap->avl, header->size, header);
}

void kalloc_heap_print(const Kalloc_Heap* heap) {
	screen_print("*** PRINTING HEAP STATE ***\n");
	int counter = 0;
	Kalloc_Heap_Header* header = (Kalloc_Heap_Header*)heap->initial_addr;
	while ((unsigned char*)header < ((unsigned char*)heap->initial_addr + heap->size)) {
		screen_print("Element: ");
		screen_print_u32(++counter);
		screen_print("\n");

		screen_print("   Type: ");
		if (header->used) {
			screen_print("Block\n");
		} else {
			screen_print("Hole\n");
		}
		screen_print("   Size: ");
		screen_print_u32(header->size);
		screen_print("\n");
		header = (Kalloc_Heap_Header*)((unsigned char*)header + sizeof(Kalloc_Heap_Header) + header->size + sizeof(Kalloc_Heap_Footer));
	}
	screen_print("******\n");
}
#else
#define KERNEL_PAGES 100
u32 k_addr;
void kalloc_heap_create(Kalloc_Heap* heap, u32 initial_addr, u32 initial_pages) {
	for (int i = 0; i < KERNEL_PAGES; ++i) {
		paging_create_page_with_any_frame(initial_addr / PAGE_SIZE + i);
	}
	k_addr = initial_addr;
}

void* kalloc_heap_alloc(Kalloc_Heap* heap, u32 size) {
	k_addr += size;
	return k_addr - size;
}

void kalloc_heap_free(Kalloc_Heap* heap, void* ptr) {
}

void kalloc_heap_print(const Kalloc_Heap* heap) {
}
#endif
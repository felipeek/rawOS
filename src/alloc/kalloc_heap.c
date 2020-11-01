#include "kalloc_heap.h"
#include "../util/util.h"
#include "../util/printf.h"
#include "../paging.h"

#define COMPLEX_HEAP_ENABLED
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

#ifdef COMPLEX_HEAP_ENABLED
void kalloc_heap_create(Kalloc_Heap* heap, u32 initial_addr, u32 initial_pages) {
	util_assert("kalloc: initial address must be page-alinged", initial_addr % 0x1000 == 0);
	util_assert("kalloc: insufficient number of initial pages", initial_pages * PAGE_SIZE >= sizeof(Kalloc_Heap_Footer) + sizeof(Kalloc_Heap_Header));

	for (u32 i = 0; i < NUM_PAGES_RESERVED_FOR_AVL + initial_pages; ++i) {
		paging_create_kernel_page_with_any_frame(initial_addr / PAGE_SIZE + i);
	}

	heap->avl_initial_addr = initial_addr;
	heap->initial_addr = initial_addr + NUM_PAGES_RESERVED_FOR_AVL * PAGE_SIZE;
	heap->size = initial_pages * PAGE_SIZE;

	kalloc_avl_init(&heap->avl, (void*)heap->avl_initial_addr, NUM_PAGES_RESERVED_FOR_AVL * PAGE_SIZE);

	Kalloc_Heap_Header* first_header = (Kalloc_Heap_Header*)heap->initial_addr;
	Kalloc_Heap_Footer* first_footer = (Kalloc_Heap_Footer*)((u8*)heap->initial_addr + heap->size - sizeof(Kalloc_Heap_Footer));

	first_header->magic = HEAP_HEADER_MAGIC;
	first_header->size = heap->size - sizeof(Kalloc_Heap_Header) - sizeof(Kalloc_Heap_Footer);
	first_header->used = 0;
	first_footer->header = first_header;
	first_footer->magic = HEAP_FOOTER_MAGIC;
	
	kalloc_avl_insert(&heap->avl, first_header->size, (u8*)first_header + sizeof(Kalloc_Heap_Header));
}

static void* get_aligned_address(void* address, u32 alignment) {
	u32 aligned_addr = (u32)address;
	if (alignment != 0) {
		if (aligned_addr & (alignment - 1)) {
			aligned_addr &= ~(alignment - 1);
			aligned_addr += alignment;
		}
	}

	return (void*)aligned_addr;
}

void* kalloc_heap_alloc(Kalloc_Heap* heap, u32 size, u32 alignment) {
	void* user_space = kalloc_avl_find_hole(&heap->avl, size, alignment);

	if (user_space) {
		Kalloc_Heap_Header* target_hole_header = (Kalloc_Heap_Header*)((u8*)user_space - sizeof(Kalloc_Heap_Header));
		// We found a hole
		util_assert("kalloc: found hole in inconsistent state (used == 1)", target_hole_header->used == 0);
		kalloc_avl_remove(&heap->avl, target_hole_header->size, user_space);
		u32 hole_size = target_hole_header->size;
		
		void* aligned_user_space = get_aligned_address(user_space, alignment);
		Kalloc_Heap_Header* target_aligned_hole_header = (Kalloc_Heap_Header*)((u8*)aligned_user_space - sizeof(Kalloc_Heap_Header));

		util_assert("kalloc: aligned space must be greater or equal to original space", (void*)aligned_user_space >= (void*)user_space);

		if (aligned_user_space > user_space) {
			// If the aligned address is greater than the hole address, we need to deal with the space
			// between the original hole and the alignment address.
			// For now, let's just merge this space with the previous hole/block

			// For now, if the heap is empty, we do not support the corner-case of allocating an aligned space that is not
			// aligned with the initial address of the heap. (this is a problem because there is no previous block/hole to merge)
			util_assert("kalloc: aligned-alloc is not supported when heap is empty", (u32)target_hole_header != heap->initial_addr);

			Kalloc_Heap_Footer* target_hole_footer = (Kalloc_Heap_Footer*)((u8*)target_hole_header + sizeof(Kalloc_Heap_Header) + target_hole_header->size);
			Kalloc_Heap_Footer* previous_footer = (Kalloc_Heap_Footer*)((u8*)target_hole_header - sizeof(Kalloc_Heap_Footer));
			Kalloc_Heap_Header* previous_header = previous_footer->header;

			Kalloc_Heap_Footer* new_previous_footer = (Kalloc_Heap_Footer*)((u8*)target_aligned_hole_header - sizeof(Kalloc_Heap_Footer));
			util_assert("kalloc: new footer address must be bigger than previous footer address", (void*)new_previous_footer > (void*)previous_footer);
			previous_header->size += (u8*)new_previous_footer - (u8*)previous_footer;
			new_previous_footer->header = previous_header;
			new_previous_footer->magic = HEAP_FOOTER_MAGIC;

			target_hole_footer->header = target_aligned_hole_header;

			hole_size -= (u8*)aligned_user_space - (u8*)user_space;
		}

		// @Note: We can't copy magic and used from target_hole, because the memory pointed by target_hole might have been
		// damaged by the code above.
		target_aligned_hole_header->magic = HEAP_HEADER_MAGIC;
		target_aligned_hole_header->used = 0;
		target_aligned_hole_header->size = hole_size;

		target_hole_header = target_aligned_hole_header;
		user_space = aligned_user_space;

		// If we still have enough free space in the hole, we generate a new hole.
		if (target_hole_header->size > size + sizeof(Kalloc_Heap_Header) + sizeof(Kalloc_Heap_Footer)) {
			Kalloc_Heap_Header* new_block_header = target_hole_header;
			Kalloc_Heap_Footer* new_block_footer = (Kalloc_Heap_Footer*)((u8*)new_block_header + sizeof(Kalloc_Heap_Header) + size);
			Kalloc_Heap_Header* new_hole_header = (Kalloc_Heap_Header*)((u8*)new_block_footer + sizeof(Kalloc_Heap_Footer));
			Kalloc_Heap_Footer* new_hole_footer = (Kalloc_Heap_Footer*)((u8*)target_hole_header + sizeof(Kalloc_Heap_Header) + target_hole_header->size);

			new_block_header->size = size;
			new_block_header->used = 1;
			new_block_footer->magic = HEAP_FOOTER_MAGIC;
			new_block_footer->header = new_block_header;
			new_hole_header->magic = HEAP_HEADER_MAGIC;
			new_hole_header->size = hole_size - size - sizeof(Kalloc_Heap_Header) - sizeof(Kalloc_Heap_Footer);
			new_hole_header->used = 0;
			new_hole_footer->header = new_hole_header;
			kalloc_avl_insert(&heap->avl, new_hole_header->size, (u8*)new_hole_header + sizeof(Kalloc_Heap_Header));
			return user_space;
		} else {
			target_hole_header->used = 1;
			return user_space;
		}
	} else {
		// If we were not able to find a fitting hole in the AVL, we need to expand the heap.
		//printf("Expanding heap... Going from %u pages to %u pages.\n", heap->size / PAGE_SIZE, heap->size / PAGE_SIZE + 1);
		paging_create_kernel_page_with_any_frame((heap->initial_addr + heap->size) / PAGE_SIZE);

		Kalloc_Heap_Footer* last_footer = (Kalloc_Heap_Footer*)((u8*)heap->initial_addr + heap->size - sizeof(Kalloc_Heap_Footer));
		Kalloc_Heap_Header* last_header = last_footer->header;
		heap->size += PAGE_SIZE;

		if (last_header->used) {
			Kalloc_Heap_Header* new_hole_header = (Kalloc_Heap_Header*)((u8*)last_footer + sizeof(Kalloc_Heap_Footer));
			Kalloc_Heap_Footer* new_hole_footer = (Kalloc_Heap_Footer*)((u8*)new_hole_header + PAGE_SIZE - sizeof(Kalloc_Heap_Footer));
			new_hole_header->magic = HEAP_HEADER_MAGIC;
			new_hole_header->size = PAGE_SIZE - sizeof(Kalloc_Heap_Header) - sizeof(Kalloc_Heap_Footer);
			new_hole_header->used = 0;
			new_hole_footer->magic = HEAP_FOOTER_MAGIC;
			new_hole_footer->header = new_hole_header;

			kalloc_avl_insert(&heap->avl, new_hole_header->size, (u8*)new_hole_header + sizeof(Kalloc_Heap_Header));
		} else {
			Kalloc_Heap_Footer* new_footer = (Kalloc_Heap_Footer*)((u8*)heap->initial_addr + heap->size - sizeof(Kalloc_Heap_Footer));
			new_footer->magic = HEAP_FOOTER_MAGIC;
			new_footer->header = last_header;

			kalloc_avl_remove(&heap->avl, last_header->size, (u8*)last_header + sizeof(Kalloc_Heap_Header));
			last_header->size += PAGE_SIZE;
			kalloc_avl_insert(&heap->avl, last_header->size, (u8*)last_header + sizeof(Kalloc_Heap_Header));
		}
		return kalloc_heap_alloc(heap, size, alignment);
	}
}

void kalloc_heap_free(Kalloc_Heap* heap, void* ptr) {
	Kalloc_Heap_Header* header = (Kalloc_Heap_Header*)((u8*)ptr - sizeof(Kalloc_Heap_Header));
	Kalloc_Heap_Footer* footer = (Kalloc_Heap_Footer*)((u8*)ptr + header->size);
	util_assert("found block in inconsistent state (used == 0)", header->used == 1);

	// Check if there is a previous header
	if ((u32)header != heap->initial_addr) {
		Kalloc_Heap_Footer* previous_footer = (Kalloc_Heap_Footer*)((u8*)header - sizeof(Kalloc_Heap_Footer));
		Kalloc_Heap_Header* previous_header = previous_footer->header;

		// Check whether previous header defines a hole
		if (!previous_header->used) {
			// Remove this hole from AVL, because we will merge it with the new hole.
			kalloc_avl_remove(&heap->avl, previous_header->size, (u8*)previous_header + sizeof(Kalloc_Heap_Header));
			previous_header->size += header->size + sizeof(Kalloc_Heap_Header) + sizeof(Kalloc_Heap_Footer);
			footer->header = previous_header;
			header = previous_header;
		}
	}

	// Check if there is a next header
	if ((u32)footer != heap->initial_addr + heap->size - sizeof(Kalloc_Heap_Footer)) {
		Kalloc_Heap_Header* next_header = (Kalloc_Heap_Header*)((u8*)footer + sizeof(Kalloc_Heap_Footer));
		Kalloc_Heap_Footer* next_footer = (Kalloc_Heap_Footer*)((u8*)next_header + sizeof(Kalloc_Heap_Header) + next_header->size);

		// Check whether next header defines a hole
		if (!next_header->used) {
			// Remove this hole from AVL, because we will merge it with the new hole.
			kalloc_avl_remove(&heap->avl, next_header->size, (u8*)next_header + sizeof(Kalloc_Heap_Header));
			
			header->size += next_header->size + sizeof(Kalloc_Heap_Header) + sizeof(Kalloc_Heap_Footer);
			next_footer->header = header;
			footer = next_footer;
		}
	}

	header->used = 0;
	kalloc_avl_insert(&heap->avl, header->size, (u8*)header + sizeof(Kalloc_Heap_Header));
}

void kalloc_heap_print(const Kalloc_Heap* heap) {
	printf("*** PRINTING HEAP STATE ***\n");
	u32 counter = 0;
	Kalloc_Heap_Header* header = (Kalloc_Heap_Header*)heap->initial_addr;
	while ((u8*)header < ((u8*)heap->initial_addr + heap->size)) {
		printf("Element: %u\n", ++counter);

		printf("   Type: ");
		if (header->used) {
			printf("Block\n");
		} else {
			printf("Hole\n");
		}
		printf("   Size: %u\n", header->size);
		header = (Kalloc_Heap_Header*)((u8*)header + sizeof(Kalloc_Heap_Header) + header->size + sizeof(Kalloc_Heap_Footer));
	}
	printf("******\n");
}
#else
#define KERNEL_PAGES 100
u32 k_addr;
void kalloc_heap_create(Kalloc_Heap* heap, u32 initial_addr, u32 initial_pages) {
	for (u32 i = 0; i < KERNEL_PAGES; ++i) {
		paging_create_kernel_page_with_any_frame(initial_addr / PAGE_SIZE + i);
	}
	k_addr = initial_addr;
}

void* kalloc_heap_alloc(Kalloc_Heap* heap, u32 size, u32 alignment) {
	u32 aligned_addr = (u32)k_addr;
	if (alignment != 0) {
		if (aligned_addr & (alignment - 1)) {
			aligned_addr &= ~(alignment - 1);
			aligned_addr += alignment;
		}
	}
	k_addr = aligned_addr;

	k_addr += size;
	return (void*)(k_addr - size);
}

void kalloc_heap_free(Kalloc_Heap* heap, void* ptr) {
}

void kalloc_heap_print(const Kalloc_Heap* heap) {
}
#endif
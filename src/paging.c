#include "paging.h"
#include "asm/paging.h"
#include "util/util.h"
#include "interrupt.h"
#include "util/bitmap.h"
#include "screen.h"

#define KERNEL_STACK_RESERVED_PAGES 8
// We can only use 3GB of PHYSICAL_RAM_SIZE because of the 3GB barrier imposed by the hardware
// https://en.wikipedia.org/wiki/3_GB_barrier
#define PHYSICAL_RAM_SIZE 0xC0000000
// The address in which the kernel stack is stored.
// @NOTE: If this value is changed, we need to change in kernel_entry.asm aswell.
// @TODO: Make both places consume the same constant
#define KERNEL_STACK_ADDRESS 0xC0000000
#define AVAILABLE_FRAMES_NUM (PHYSICAL_RAM_SIZE / 0x1000)
#define KERNEL_PAGE_TABLES_ADDRESS 0x00100000

// Each x86 page has 4KB (default)
// Each page table also has 4KB. Each page table entry occupies 4 bytes (32 bits). Therefore, a single page table can
// store pointers to 1024 pages (4096 / 4 bytes), which translates to 4MB in total.
// Each page directory also has 4KB. Each page directory entry occupies 4 byte (32 bits). Therefore, a page directory can
// store pointers to 1024 page tables (4096 / 4 bytes), which translates to 4MB * 4MB = 4GB in total (the entire 32-bit address space).
//
// The desired page directory location shall be copied to the CR3 register to inform the processor which page directory to use.

/*
	rawOS Kernel Address Space Layout

	0xC0000000    | Start of Stack
	              | Stack Space
	----------    | Free Space
	              | Heap Space
	0x00500000    | Start of Heap
	              | Reserved for Kernel Page Tables
    0x00100000    |
                  | Not Used
    0x000C0000    |
	              | Reserved for Video Memory
    0x000A0000    |
	              | Reserved for Kernel Code + Kernel Data
	0x00000000    |
*/


// The page entry, as defined by Intel in the x86 architecture
typedef struct {
	u32 present : 1;            // If set, page is present in RAM
	u32 writable : 1;           // If set, page is writable. Otherwise, page is read-only. This does not apply when code is running in kernel-mode (unless a flag in CR0 is set).
	u32 user_mode : 1;          // If set, user-mode page. Else it is kernel-mode page. User-mode code can't read or write to kernel pages.
	u32 reserved : 2;           // Reserved for the CPU. Cannot be changed.
	u32 accessed : 1;           // Gets set if the page is accessed (by the CPU)
	u32 dirty : 1;              // Gets set if the page has been written to (by the CPU)
	u32 reserved2 : 2;          // Reserved for the CPU. Cannot be changed.
	u32 available : 3;          // Unused and available for kernel use.
	u32 frame_address : 20;     // The high 20 bits of the frame address in RAM.
} Page_Entry;

// The page table. Each page table contains 1024 pages.
typedef struct {
	Page_Entry pages[1024];
} Page_Table;

// The page directory. Each page directory contains 1024 page tables.
typedef struct {
	// Pointers to each page table
	Page_Table* tables[1024];
	// The representation of the page directory as expected by x86 (this is loaded in the CR3 register)
	u32 tables_x86_representation[1024];
} Page_Directory;

unsigned char available_frames_bitmap_data[AVAILABLE_FRAMES_NUM / 8];
typedef struct {
	Bitmap available_frames;
	Page_Directory* page_directory;
} Paging;

Paging paging;

// The end section is defined in link.ld
// This global variable basically points to the end of memory reserved to the kernel.
extern u32 end;
// This global variable will start by pointing to the end of code+data, but it can potentially grow
// if we need to allocate generic stuff before enabling paging
u32 final_kernel_code_data_addr = (u32)&end;

/* ******************** */
/* PRE-PAGING FUNCTIONS */
/* ******************** */

// THIS FUNCTION SHOULD ONLY BE USED BEFORE PAGING IS ENABLED.
// This function maps a given page (page_num) to a given frame (frame_num).
// The page entry and page table will be prepared for when paging is enabled.
// If a page table needs to be created, its virtual address will be already the correct one (it will be under the 
// reserved space that we have for page tables in the kernel address space)
// The physical address of the page table, however, will be *EQUAL TO ITS VIRTUAL ADDRESS*. In other words, we are using an identity map.
// We do that because if we were to pick the first frame available in the bitmap, we could pick a frame that will be needed in the future for the
// identity maps that needs to be created when paging is enabled. (e.g. the kernel code/data)
// Using an identity map for the page tables here is also helpful because the page_directory->tables[*] pointer will be valid both before and after
// paging is enabled, since the physical address is equal to the virtual address.
// Note that this function assumes that all the physical addresses that fall under the address range reserved for page tables
// (in the kernel virtual address space) are free and can be used freely (because, again, we are using an identity map).
static void create_pre_paging_mapping(u32 page_num, u32 frame_num) {
	u32 page_table_index = page_num / 1024;
	u32 page_num_within_table = page_num % 1024;

	if (!paging.page_directory->tables[page_table_index]) {
		// The page address. Serves for both virtual and physical address, since we are using an identity map.
		u32 page_address = KERNEL_PAGE_TABLES_ADDRESS + page_table_index * 0x1000;
		// The page number. Serves for both virtual and physical page, since we are using an identity map.
		u32 page_num = page_address / 0x1000;
		paging.page_directory->tables[page_table_index] = (Page_Table*)page_address;
		paging.page_directory->tables_x86_representation[page_table_index] = (u32)(paging.page_directory->tables[page_table_index]) | 0x7; // PRESENT, RW, US

		util_memset(paging.page_directory->tables[page_table_index], 0, sizeof(Page_Table));

		// The new page table also needs a frame... In this case we call this function recursively, mapping the just configured
		// virtual address to its corresponding physical address (they are equal)
		// The recursive call will create the frame (i.e. set the bitmap and configure the page entry in the page table that
		// will contain the frame of the page table that we are currently creating :))
		// Note that we can have a recursive call with depth even more bigger.
		create_pre_paging_mapping(page_num, page_num);

		screen_print("Allocating new table ");
		screen_print_u32(page_table_index);
		screen_print("\n");
	}

	Page_Entry* page_entry = &paging.page_directory->tables[page_table_index]->pages[page_num_within_table];
	util_memset(page_entry, 0, sizeof(Page_Entry));
	bitmap_set(&paging.available_frames, frame_num);
	page_entry->present = 1;
	page_entry->user_mode = 0;  // for now all frames are kernel frames
	page_entry->writable = 1;   // for now all pages are writable
	page_entry->frame_address = frame_num;
}

// Reserve space after the end of the kernel code+data memory segment.
// This doesn't do anything special, just mantains a pointer that is incremented.
static void* reserve_pre_paging_space(u32 size) {
	void* addr = (void*)final_kernel_code_data_addr;
	final_kernel_code_data_addr += size;
	util_memset(addr, 0, size);
	return addr;
}

// Reserve page-aligned space after the end of the kernel code+data memory segment.
// This doesn't do anything special, just mantains a pointer that is incremented.
static void* reserve_pre_paging_aligned_space(u32 size) {
	if (final_kernel_code_data_addr & 0xFFFFF000) {
		final_kernel_code_data_addr &= 0xFFFFF000;
		final_kernel_code_data_addr += 0x1000;
	}
	void* addr = (void*)final_kernel_code_data_addr;
	final_kernel_code_data_addr += size;
	util_memset(addr, 0, size);
	return addr;
}

/* ******************** */

static int page_exist(u32 page_num) {
	u32 page_table_index = page_num / 1024;
	u32 page_num_within_table = page_num % 1024;
	if (paging.page_directory->tables[page_table_index]) {
		return paging.page_directory->tables[page_table_index]->pages[page_num_within_table].present;
	}
	return 0;
}

// This function creates a virtual page and allocates a frame to it.
// Can only be called if the given virtual page is not being used.
// Returns allocd frame
static u32 create_page_with_any_frame(u32 page_num) {
	u32 page_table_index = page_num / 1024;
	u32 page_num_within_table = page_num % 1024;

	if (!paging.page_directory->tables[page_table_index]) {
		u32 virtual_page_address = KERNEL_PAGE_TABLES_ADDRESS + page_table_index * 0x1000;
		u32 virtual_page_num = virtual_page_address / 0x1000;

		u32 allocd_frame = create_page_with_any_frame(virtual_page_num);
		
		// The new page table also needs a frame. Calls this function recursively to get the frame.
		paging.page_directory->tables[page_table_index] = (Page_Table*)virtual_page_address;
		paging.page_directory->tables_x86_representation[page_table_index] = (u32)(allocd_frame * 0x1000) | 0x7; // PRESENT, RW, US

		// @TODO: Solve impossible problem
		//util_memset(paging.page_directory->tables[page_table_index], 0, sizeof(Page_Table));

		screen_print("Allocating new table ");
		screen_print_u32(page_table_index);
		screen_print("\n");
	}

	Page_Entry* page_entry = &paging.page_directory->tables[page_table_index]->pages[page_num_within_table];
	util_assert("Trying to create page that already exists!", !page_entry->present);

	u32 allocd_frame = bitmap_get_first_clear(&paging.available_frames);
	screen_print("allocd_frame: ");
	screen_print_ptr((void*)allocd_frame);
	screen_print("\n");
	bitmap_set(&paging.available_frames, allocd_frame);
	page_entry->present = 1;
	page_entry->user_mode = 0;  // for now all frames are kernel frames
	page_entry->writable = 1;   // for now all pages are writable
	page_entry->frame_address = allocd_frame;
	return allocd_frame;
}

// Gets a page from a page directory. The page must already exist.
static Page_Entry* get_page(u32 page_num) {
	u32 page_table_index = page_num / 1024;
	u32 page_num_within_table = page_num % 1024;

	util_assert("Trying to get a page from a table that is not created!", paging.page_directory->tables[page_table_index] != 0);
	Page_Entry* page_entry = &paging.page_directory->tables[page_table_index]->pages[page_num_within_table];
	util_assert("Trying to get a page that doesn't exist!", page_entry->present);
	return page_entry;
}

static void page_fault_handler(const Interrupt_Handler_Args* args) {
	u32 faulting_addr = paging_get_faulting_address();

	// The error code gives us details of what happened.
	int present = !(args->err_code & 0x1);   // Page not present
	int rw = args->err_code & 0x2;           // Write operation?
	int us = args->err_code & 0x4;           // Processor was in user-mode?
	int reserved = args->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
	int id = args->err_code & 0x10;          // Caused by an instruction fetch?

	// Output an error message.
	screen_print("Page fault! ( ");
	if (present) screen_print("present ");
	if (rw) screen_print("read-only ");
	if (us) screen_print("user-mode ");
	if (reserved) screen_print("reserved ");
	screen_print(") at 0x");
	screen_print_u32(faulting_addr);
	screen_print("\n");
	util_panic("Page fault");
}

static void print_all_present_pages() {
	screen_print("Present pages:\n");
	for (u32 i = 0; i < 1024; ++i) {
		Page_Table* current_page_table = paging.page_directory->tables[i];
		if (current_page_table) {
			for (u32 j = 0; j < 1024; ++j) {
				Page_Entry* page_entry = &current_page_table->pages[j];
				if (page_entry->present) {
					screen_print_ptr((void*)(0x400000 * i + 0x1000 * j));
					screen_print(" ");
				}
			}
		}
	}
	screen_print("\n");
}

void paging_init() {
	bitmap_init(&paging.available_frames, available_frames_bitmap_data, AVAILABLE_FRAMES_NUM / 8);

	// We allocate a page_directory for the kernel.
	paging.page_directory = reserve_pre_paging_aligned_space(sizeof(Page_Directory));

	// First, we reserve N pages for the kernel stack (N is KERNEL_STACK_RESERVED_PAGES)
	// We start at KERNEL_STACK_ADDRESS and go down.
	for (u32 i = 0; i < KERNEL_STACK_RESERVED_PAGES; ++i) {
		u32 page_num = (KERNEL_STACK_ADDRESS / 0x1000) - i;
		create_pre_paging_mapping(page_num, page_num);
		++page_num;
	}

	// Here we perform an identity map on the video memory.
	// In the future, we might wanna reserve another space in the kernel address space for the video memory.
	for (u32 i = 0xA0000; i < 0xC0000; i += 0x1000) {
		u32 page_num = i / 0x1000;
		create_pre_paging_mapping(page_num, page_num);
		++page_num;
	}

	// Here, we create pages for all the kernel code and data that is currently in RAM.
	// We use an identity map (VIRTUAL ADDRESS = PHYSICAL ADDRESS), meaning that the pages will point to frames with same address.
	// Also note that 'allocate_frame_to_get_page' might need to allocate memory for the page tables.
	// Because of this, 'kmalloc_addr' might change in the middle of the loop. We need to be sure that we are
	// accounting for that, since all kmalloc/kcalloc'd stuff need to be part of the identity map.
	// @NOTE: If the kernel grows enough, this might invade the address space reserved for the video memory (see above)
	// If this happens, we will need to change the virtual space reserved for the video memory, instead of performing an identity map.
	for (u32 i = 0; i < final_kernel_code_data_addr; i += 0x1000) {
		u32 page_num = i / 0x1000;
		create_pre_paging_mapping(page_num, page_num);
		++page_num;
	}

	// Finally, we enable paging using the kernel page directory that we just created.
	paging_switch_page_directory(paging.page_directory->tables_x86_representation);

	interrupt_register_handler(page_fault_handler, 14);

	//print_all_present_pages();

	// 0x07867000
	create_page_with_any_frame(0xABDE);
	u8 a = *(u8*)0xABDF000;
}
#include "paging.h"
#include "asm/paging.h"
#include "util/util.h"
#include "interrupt.h"
#include "util/bitmap.h"
#include "util/printf.h"
#include "alloc/kalloc.h"

#define KERNEL_STACK_RESERVED_PAGES 2048
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

u8 available_frames_bitmap_data[AVAILABLE_FRAMES_NUM / 8];
typedef struct {
	Bitmap available_frames;
	Page_Directory* kernel_page_directory;
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

static void create_pre_paging_mapping(u32 page_num, u32 frame_num);

static void create_pre_paging_page_table(u32 page_table_index) {
	// The page address. Serves for both virtual and physical address, since we are using an identity map.
	u32 page_address = KERNEL_PAGE_TABLES_ADDRESS + page_table_index * 0x1000;
	// The page number. Serves for both virtual and physical page, since we are using an identity map.
	u32 page_num = page_address / 0x1000;
	paging.kernel_page_directory->tables[page_table_index] = (Page_Table*)page_address;
	paging.kernel_page_directory->tables_x86_representation[page_table_index] = (u32)(paging.kernel_page_directory->tables[page_table_index]) | 0x7; // PRESENT, RW, US

	util_memset(paging.kernel_page_directory->tables[page_table_index], 0, sizeof(Page_Table));

	// The new page table also needs a frame... In this case we call this function recursively, mapping the just configured
	// virtual address to its corresponding physical address (they are equal)
	// The recursive call will create the frame (i.e. set the bitmap and configure the page entry in the page table that
	// will contain the frame of the page table that we are currently creating :))
	// Note that we can have a recursive call with depth even more bigger.
	create_pre_paging_mapping(page_num, page_num);

	printf("Allocating new table %u\n", page_table_index);
}

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

	if (!paging.kernel_page_directory->tables[page_table_index]) {
		create_pre_paging_page_table(page_table_index);
	}

	Page_Entry* page_entry = &paging.kernel_page_directory->tables[page_table_index]->pages[page_num_within_table];
	util_memset(page_entry, 0, sizeof(Page_Entry));
	bitmap_set(&paging.available_frames, frame_num);
	page_entry->present = 1;
	page_entry->user_mode = 0;  // kernel-mode pages
	page_entry->writable = 1;   // for now all pages are writable
	page_entry->frame_address_20_bits = frame_num;
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

static u32 get_physical_address_of_virtual_address(const Page_Directory* page_directory, u32 virtual_addr) {
	u32 page_num = virtual_addr / 4096;
	u32 page_offset = virtual_addr % 4096;
	u32 table_num = page_num / 1024;
	u32 page_index_within_table = page_num % 1024;
	return page_directory->tables[table_num]->pages[page_index_within_table].frame_address_20_bits * 0x1000 + page_offset;
}

u32 paging_get_page_directory_x86_tables_frame_address(const Page_Directory* page_directory) {
	return get_physical_address_of_virtual_address(paging.kernel_page_directory, (u32)page_directory)
		+ OFFSETOF(Page_Directory, tables_x86_representation);
}

u32 paging_get_page_frame_address(const Page_Directory* page_directory, u32 page_num) {
	return get_physical_address_of_virtual_address(page_directory, page_num * 0x1000);
}

// Clone the page_directory of an existing process.
// The kernel is always linked to the first 1GB of the address space.
// The process data, which is part of 1GB-4GB address space range, is copied, not linked.
Page_Directory* paging_clone_page_directory_for_new_process(const Page_Directory* page_directory) {
	// x86 demands that the page directory is 0x1000 aligned.
	// Obvious question is: we are making the virtual address 0x1000 aligned, how does it help with
	// regards to the physical addr? (which is the one consumed by x86)
	// Answer: As long as the initial virtual address of our heap is 0x1000 aligned this should work
	// because the heap will always start by allocating a brand new page, so both virtual address and physical adresses
	// will always be aligned together.
	Page_Directory* cloned_page_directory = kalloc_alloc_aligned(sizeof(Page_Directory), 0x1000);
	util_memset(cloned_page_directory, 0, sizeof(Page_Directory));

	// @TODO: IMPORTANT: If the kernel creates a new page table, dynamically, we need to add it to all existing address spaces !!!

	// We start by copying all page tables from 1GB to 4GB.
	for (u32 i = 1024 / 4; i < 1024; ++i) {
		// If the page table exists
		if (page_directory->tables[i]) {
			// x86 demands that the page table is 0x1000 aligned.
			// Obvious question is: we are making the virtual address 0x1000 aligned, how does it help with
			// regards to the physical addr? (which is the one consumed by x86)
			// Answer: As long as the initial virtual address of our heap is 0x1000 aligned this should work
			// because the heap will always start by allocating a brand new page, so both virtual address and physical adresses
			// will always be aligned together.
			Page_Table* copied_page_table = kalloc_alloc_aligned(sizeof(Page_Table), 0x1000);
			util_memset(copied_page_table, 0, sizeof(Page_Table));

			for (u32 j = 0; j < 1024; ++j) {
				Page_Entry* current_page_entry = &page_directory->tables[i]->pages[j];
				if (current_page_entry->present) {
					// For now, the new page entry receives the same attributes as the one being cloned
					copied_page_table->pages[j] = *current_page_entry;
					// Allocate a new frame for the new page
					u32 allocd_frame = bitmap_get_first_clear(&paging.available_frames);
					bitmap_set(&paging.available_frames, allocd_frame);
					paging_copy_frame(allocd_frame * 0x1000, current_page_entry->frame_address_20_bits << 12);
					// Update the page entry to point to the new frame address
					copied_page_table->pages[j].frame_address_20_bits = allocd_frame;
					// Force page to be user-mode
					copied_page_table->pages[j].user_mode = 1;
				}
			}

			// Assign tables 'i' of the new page directory to the table we just created
			cloned_page_directory->tables[i] = copied_page_table;
			// Get the physical address of the table that we just created
			u32 copied_page_table_physical_address = get_physical_address_of_virtual_address(paging.kernel_page_directory,
				(u32)copied_page_table);
			// Set the tables_x86_representation, expected by x86, to the physical address just calculated (0x7 because user-mode=1)
			cloned_page_directory->tables_x86_representation[i] = copied_page_table_physical_address | 0x7; // PRESENT, RW, US
		}
	}
	
	// We finish by linking the kernel in the new address space
	// We link all page tables from 0 to 1024/4, so we account for the first 1GB of the address space.
	for (u32 i = 0; i < 1024 / 4; ++i) {
		// If the page table exists
		if (page_directory->tables[i]) {
			// Link (don't copy) the table
			cloned_page_directory->tables[i] = page_directory->tables[i];
			cloned_page_directory->tables_x86_representation[i] = page_directory->tables_x86_representation[i];
		}
	}

	return cloned_page_directory;
}

Page_Directory* paging_clone_page_directory_for_new_process_2(const Page_Directory* page_directory) {
	// x86 demands that the page directory is 0x1000 aligned.
	// Obvious question is: we are making the virtual address 0x1000 aligned, how does it help with
	// regards to the physical addr? (which is the one consumed by x86)
	// Answer: As long as the initial virtual address of our heap is 0x1000 aligned this should work
	// because the heap will always start by allocating a brand new page, so both virtual address and physical adresses
	// will always be aligned together.
	Page_Directory* cloned_page_directory = kalloc_alloc_aligned(sizeof(Page_Directory), 0x1000);
	util_memset(cloned_page_directory, 0, sizeof(Page_Directory));

	// @TODO: IMPORTANT: If the kernel creates a new page table, dynamically, we need to add it to all existing address spaces !!!

	// We start by copying all page tables from 1GB to 4GB.
	#if 0
	for (u32 i = 1024 / 4; i < 1024; ++i) {
		// If the page table exists
		if (page_directory->tables[i]) {
			// x86 demands that the page table is 0x1000 aligned.
			// Obvious question is: we are making the virtual address 0x1000 aligned, how does it help with
			// regards to the physical addr? (which is the one consumed by x86)
			// Answer: As long as the initial virtual address of our heap is 0x1000 aligned this should work
			// because the heap will always start by allocating a brand new page, so both virtual address and physical adresses
			// will always be aligned together.
			Page_Table* copied_page_table = kalloc_alloc_aligned(sizeof(Page_Table), 0x1000);
			util_memset(copied_page_table, 0, sizeof(Page_Table));

			for (u32 j = 0; j < 1024; ++j) {
				Page_Entry* current_page_entry = &page_directory->tables[i]->pages[j];
				if (current_page_entry->present) {
					// For now, the new page entry receives the same attributes as the one being cloned
					copied_page_table->pages[j] = *current_page_entry;
					// Allocate a new frame for the new page
					u32 allocd_frame = bitmap_get_first_clear(&paging.available_frames);
					bitmap_set(&paging.available_frames, allocd_frame);
					// Calculate the frame's physical address of the page that we are cloning
					u32 current_page_frame_address = get_physical_address_of_virtual_address(page_directory,
						current_page_entry->frame_address_20_bits * 0x1000);
					// Copy the frame that we are cloning to the frame that we just allocated
					paging_copy_frame(allocd_frame * 0x1000, current_page_frame_address);
					// Update the page entry to point to the new frame address
					copied_page_table->pages[j].frame_address_20_bits = allocd_frame;
					// Force page to be user-mode
					copied_page_table->pages[j].user_mode = 1;
				}
			}

			// Assign tables 'i' of the new page directory to the table we just created
			cloned_page_directory->tables[i] = copied_page_table;
			// Get the physical address of the table that we just created
			u32 copied_page_table_physical_address = get_physical_address_of_virtual_address(paging.kernel_page_directory,
				(u32)copied_page_table);
			// Set the tables_x86_representation, expected by x86, to the physical address just calculated (0x7 because user-mode=1)
			cloned_page_directory->tables_x86_representation[i] = copied_page_table_physical_address | 0x7; // PRESENT, RW, US
		}
	}
	#endif
	
	// We finish by linking the kernel in the new address space
	// We link all page tables from 0 to 1024/4, so we account for the first 1GB of the address space.
	for (u32 i = 0; i < 1024; ++i) {
		// If the page table exists
		if (page_directory->tables[i]) {
			// Link (don't copy) the table
			cloned_page_directory->tables[i] = page_directory->tables[i];
			cloned_page_directory->tables_x86_representation[i] = page_directory->tables_x86_representation[i];
		}
	}

	return cloned_page_directory;
}

static s32 page_exist(const Page_Directory* page_directory, u32 page_num) {
	u32 page_table_index = page_num / 1024;
	u32 page_num_within_table = page_num % 1024;
	if (page_directory->tables[page_table_index]) {
		return page_directory->tables[page_table_index]->pages[page_num_within_table].present;
	}
	return 0;
}

// This function creates a virtual page and allocates a frame to it.
// Can only be called if the given virtual page is not being used.
// Returns allocd frame
u32 paging_create_page_with_any_frame(Page_Directory* page_directory, u32 page_num, u32 user_mode) {
	//printf("allocating page num %u\n", page_num);
	u32 page_table_index = page_num / 1024;
	u32 page_num_within_table = page_num % 1024;

	if (!page_directory->tables[page_table_index]) {
		u32 virtual_page_address = KERNEL_PAGE_TABLES_ADDRESS + page_table_index * 0x1000;
		u32 virtual_page_num = virtual_page_address / 0x1000;

		page_directory->tables[page_table_index] = (Page_Table*)virtual_page_address;
		// The new page table also needs a frame. Calls this function recursively to get the frame.
		// @NOTE: The key here is that we know for sure that the page table used to store 'virtual_page_num' will ALWAYS exist.
		// This is because we always pre-allocate all page tables that may be used to store other page tables.
		// For this reason, we don't need to worry about the same page-table being referenced multiple times during recursion.
		u32 allocd_frame = paging_create_page_with_any_frame(page_directory, virtual_page_num, 0);
		page_directory->tables_x86_representation[page_table_index] = (u32)(allocd_frame * 0x1000) | 0x7; // PRESENT, RW, US
		util_memset(page_directory->tables[page_table_index], 0, sizeof(Page_Table));

		printf("Allocating new table %u\n", page_table_index);
	}

	Page_Entry* page_entry = &page_directory->tables[page_table_index]->pages[page_num_within_table];
	util_assert("Trying to create page that already exists!", !page_entry->present);

	u32 allocd_frame = bitmap_get_first_clear(&paging.available_frames);
	bitmap_set(&paging.available_frames, allocd_frame);
	page_entry->present = 1;
	page_entry->user_mode = user_mode;
	page_entry->writable = 1;   // for now all pages are writable
	page_entry->frame_address_20_bits = allocd_frame;

	// Double-check whether this frame is addressable!
	// If we pick a physical page that is not addressable (because, for example, it is reserved for MMIO),
	// then we will not be able to write to this page
	// ... and it will be extremely hard to debug what is going on :)
	u8* test = (u8*)(page_num * 0x1000);
	*test = 0xAB;
	//printf("Page: %x\n", allocd_frame * 0x1000);
	util_assert("A non-addressable frame was chosen!", *test == 0xAB);

	return allocd_frame;
}

u32 paging_create_kernel_page_with_any_frame(u32 page_num) {
	return paging_create_page_with_any_frame(paging.kernel_page_directory, page_num, 0);
}

// Gets a page from a page directory. The page must already exist.
static Page_Entry* get_page(const Page_Directory* page_directory, u32 page_num) {
	u32 page_table_index = page_num / 1024;
	u32 page_num_within_table = page_num % 1024;

	util_assert("Trying to get a page from a table that is not created!", page_directory->tables[page_table_index] != 0);
	Page_Entry* page_entry = &page_directory->tables[page_table_index]->pages[page_num_within_table];
	util_assert("Trying to get a page that doesn't exist!", page_entry->present);
	return page_entry;
}

static void page_fault_handler(Interrupt_Handler_Args* args) {
	u32 faulting_addr = paging_get_faulting_address();

	// The error code gives us details of what happened.
	s32 present = !(args->err_code & 0x1);   // Page not present
	s32 rw = args->err_code & 0x2;           // Write operation?
	s32 us = args->err_code & 0x4;           // Processor was in user-mode?
	s32 reserved = args->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
	s32 id = args->err_code & 0x10;          // Caused by an instruction fetch?

	// Output an error message.
	printf("Page fault! ( ");
	if (present) printf("present ");
	if (rw) printf("read-only ");
	if (us) printf("user-mode ");
	if (reserved) printf("reserved ");
	printf(") at 0x%x\n", faulting_addr);
	util_panic("Page fault");
}

static void print_all_present_pages(const Page_Directory* page_directory) {
	printf("Present pages:\n");
	for (u32 i = 0; i < 1024; ++i) {
		Page_Table* current_page_table = page_directory->tables[i];
		if (current_page_table) {
			for (u32 j = 0; j < 1024; ++j) {
				Page_Entry* page_entry = &current_page_table->pages[j];
				if (page_entry->present) {
					printf("%x ", (void*)(0x400000 * i + 0x1000 * j));
				}
			}
		}
	}
	printf("\n");
}

void paging_init() {
	bitmap_init(&paging.available_frames, available_frames_bitmap_data, AVAILABLE_FRAMES_NUM / 8);

	// We allocate a page_directory for the kernel.
	paging.kernel_page_directory = reserve_pre_paging_aligned_space(sizeof(Page_Directory));

	// First, we pre-allocate some necessary page-tables.
	// We are basically allocating all the page-tables that may contain pages used by other page-tables.
	// e.g. if our KERNEL_PAGE_TABLE_ADDRESS is 0x100000, then we know for sure that the page table virtual space will
	// be reserved from 0x100000 to 0x500000.
	// In this case, we know that all page tables from 0x100000 to 0x400000 will be stored as pages located in the
	// page table 0, whereas all pages tables from 0x400000 to 0x500000 will be stored as pages located in the page table 1.
	// So, we pre-allocate page-table 0 and 1.
	// This is necessary to avoid an "chicken-and-egg" problem after paging is enabled. For example, let's say that we enable paging,
	// but we didn't pre-allocate page-table 1. Then, we need to create a page that falls in page-table 1. In this case, we then try
	// to allocate a page to store page table 1, but then we find out that the page table 1 page also falls in page table 1! In this
	// case we are screwed, since we can never write to that page, because we don't have a virtual address to use.
	util_assert("The virtual address of kernel page tables must be multiple of 0x1000", KERNEL_PAGE_TABLES_ADDRESS % 0x1000 == 0);
	u32 first_page_table_index = (KERNEL_PAGE_TABLES_ADDRESS / 0x1000) / 1024;
	u32 last_page_table_index = ((KERNEL_PAGE_TABLES_ADDRESS + 0x400000) / 0x1000) / 1024;
	printf("Pre-creating page tables from %u to %u.\n", first_page_table_index, last_page_table_index);
	for (u32 i = first_page_table_index; i <= last_page_table_index; ++i) {
		create_pre_paging_page_table(i);
	}

	// First, we reserve N pages for the kernel stack (N is KERNEL_STACK_RESERVED_PAGES)
	// We start at KERNEL_STACK_ADDRESS and go down.
	for (u32 i = 0; i < KERNEL_STACK_RESERVED_PAGES; ++i) {
		u32 page_num = (KERNEL_STACK_ADDRESS / 0x1000) - i;
		create_pre_paging_mapping(page_num, page_num);
	}

	// Here we perform an identity map on the video memory.
	// In the future, we might wanna reserve another space in the kernel address space for the video memory.
	for (u32 i = 0xA0000; i < 0xC0000; i += 0x1000) {
		u32 page_num = i / 0x1000;
		create_pre_paging_mapping(page_num, page_num);
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
	}

	// @TEMPORARY: For now, let's pretend all frames below 0x100000 are used.
	// This is to avoid using this frames later.
	// Some of these frames are not addressable! So we avoid this issues.
	// @TODO: Find a good documentation about x86 protected mode memory map and understand in detail
	// which frames are reserved for MMIO, so we can treat them specially.
	for (u32 i = 0; i < 0x100000; i += 0x1000) {
		bitmap_set(&paging.available_frames, i / 0x1000);
	}

	// Finally, we enable paging using the kernel page directory that we just created.
	paging_switch_page_directory((u32)paging.kernel_page_directory->tables_x86_representation);

	interrupt_register_handler(page_fault_handler, 14);

	//print_all_present_pages();

	//create_page_with_any_frame(0x4ABDF);
	//u8 a = *(u8*)0x4ABDFFFF;	
}

Page_Directory* paging_get_kernel_page_directory() {
	return paging.kernel_page_directory;
}
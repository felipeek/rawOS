#include "paging.h"
#include "alloc/kalloc_np.h"
#include "asm/paging.h"
#include "util/util.h"
#include "interrupt.h"
#include "util/bitmap.h"

#define PHYSICAL_RAM_SIZE 1024 * 1024 * 16 // 16mb for now
#define AVAILABLE_FRAMES_NUM (PHYSICAL_RAM_SIZE / 0x1000)

// Each x86 page has 4KB (default)
// Each page table also has 4KB. Each page table entry occupies 4 bytes (32 bits). Therefore, a single page table can
// store pointers to 1024 pages (4096 / 4 bytes), which translates to 4MB in total.
// Each page directory also has 4KB. Each page directory entry occupies 4 byte (32 bits). Therefore, a page directory can
// store pointers to 1024 page tables (4096 / 4 bytes), which translates to 4MB * 4MB = 4GB in total (the entire 32-bit address space).
//
// The desired page directory location shall be copied to the CR3 register to inform the processor which page directory to use.

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
} Paging;

Paging paging;

extern u32 kmalloc_addr;

static u32 frame_num_to_frame_addr(u32 frame_num) {
	return frame_num << 12;
}

// Gets a page from a page directory, creating a page table if necessary.
static Page_Entry* get_page(Page_Directory* page_directory, u32 page_num) {
	u32 page_table_index = page_num / 1024;
	u32 page_num_within_table = page_num % 1024;

	if (!page_directory->tables[page_table_index]) {
		// If the page table doesn't exist, we create it.
		page_directory->tables[page_table_index] = kcalloc_np_aligned(sizeof(Page_Table));
		page_directory->tables_x86_representation[page_table_index] = (u32)(page_directory->tables[page_table_index]) | 0x7; // PRESENT, RW, US
	}

	return &page_directory->tables[page_table_index]->pages[page_num_within_table];
}

// Allocates a frame a given page
static void allocate_frame_to_page(Page_Entry* page_entry) {
	if (page_entry->frame_address) {
		util_panic("trying to allocate frame for page that already has a frame assigned to it.");
	}
	u32 allocd_frame = bitmap_get_first_clear(&paging.available_frames);
	bitmap_set(&paging.available_frames, allocd_frame);
	page_entry->present = 1;
	page_entry->user_mode = 0;  // for now all frames are kernel frames
	page_entry->writable = 1;   // for now all pages are writable
	page_entry->frame_address = allocd_frame;
}

// @TEMPORARY
#include "screen.h"
void page_fault_handler(const Interrupt_Handler_Args* args) {
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

void paging_init() {
	bitmap_init(&paging.available_frames, available_frames_bitmap_data, AVAILABLE_FRAMES_NUM / 8);

	// We allocate a page_directory for the kernel.
	Page_Directory* page_directory = kcalloc_np_aligned(sizeof(Page_Directory));

	// Here, we create pages for all the kernel code and data that is currently in RAM.
	// We use an identity map (VIRTUAL ADDRESS = PHYSICAL ADDRESS), meaning that the pages will point to frames with same address.
	// Also note that 'allocate_frame_to_get_page' might need to allocate memory for the page tables.
	// Because of this, 'kmalloc_addr' might change in the middle of the loop. We need to be sure that we are
	// accounting for that, since all kmalloc/kcalloc'd stuff need to be part of the identity map.
	u32 page_num = 0;
	for (u32 i = 0; i < kmalloc_addr; i += 0x1000) {
		Page_Entry* page_entry = get_page(page_directory, page_num++);
		allocate_frame_to_page(page_entry);
	}

	while(1){}

	// Finally, we enable paging using the kernel page directory that we just created.
	paging_switch_page_directory(page_directory->tables_x86_representation);

	interrupt_register_handler(page_fault_handler, 14);
}
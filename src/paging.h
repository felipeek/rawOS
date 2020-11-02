#ifndef RAW_OS_PAGING_H
#define RAW_OS_PAGING_H
#include "common.h"

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
	u32 frame_address_20_bits : 20;     // The high 20 bits of the frame address in RAM.
} Page_Entry;

// The page table. Each page table contains 1024 pages.
typedef struct {
	Page_Entry pages[1024];
} Page_Table;

// The page directory. Each page directory contains 1024 page tables.
typedef struct {
	// Pointers to each page table
	// These pointers are stored as virtual memory pointers
	Page_Table* tables[1024];
	// The representation of the page directory as expected by x86 (this is loaded in the CR3 register)
	// This is also a list of pointers, but they are stored as physical memory pointers.
	// The only difference here is that the last three nibbles are reserved for flags
	// (they are not needed since all ptrs must be aligned to 0x1000).
	u32 tables_x86_representation[1024];
} Page_Directory;

void paging_init();
u32 paging_create_process_page_with_any_frame(Page_Directory* page_directory, u32 page_num, u32 user_mode);
u32 paging_create_kernel_page_with_any_frame(u32 page_num);
Page_Directory* paging_clone_page_directory_for_new_process(const Page_Directory* page_directory);
u32 paging_get_page_directory_x86_tables_frame_address(const Page_Directory* page_directory);
u32 paging_get_page_frame_address(const Page_Directory* page_directory, u32 page_num);
Page_Directory* paging_get_kernel_page_directory();

#endif
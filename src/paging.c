#include "paging.h"
#include "kmalloc.h"
#include "asm/paging.h"
#include "util.h"

#define PHYSICAL_RAM_SIZE 1024 * 1024 * 16 // 16mb for now
#define AVAILABLE_FRAMES_NUM (PHYSICAL_RAM_SIZE / 8)

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

typedef struct {
    // A bitmap containing the state of all available frames.
    // 0 = free
    // 1 = in use
    u32 available_frames[AVAILABLE_FRAMES_NUM];
} Paging;

Paging paging;

extern u32 kmalloc_addr;

static void set_frame_used(u32 frame_num) {
    u32 frame_bitmap_num = frame_num / 32;
    u32 frame_bitmap_bit = frame_num % 32;
    paging.available_frames[frame_bitmap_num] |= (1 << frame_bitmap_bit);
}

static void set_frame_available(u32 frame_num) {
    u32 frame_bitmap_num = frame_num / 32;
    u32 frame_bitmap_bit = frame_num % 32;
    paging.available_frames[frame_bitmap_num] &= ~(1 << frame_bitmap_bit);
}

static u32 get_first_frame_available() {
    for (u32 i = 0; i < AVAILABLE_FRAMES_NUM; ++i) {
        for (u32 j = 0; j < 32; ++j) {
            if (!(paging.available_frames[i] & (1 << j))) {
                return i * 32 + j;
            }
        }
    }

    util_panic("No RAM available to get frame!");
}

static u32 frame_num_to_frame_addr(u32 frame_num) {
    return frame_num << 12;
}

// Gets a page from a page directory, creating a page table if necessary.
static Page_Entry* get_page(Page_Directory* page_directory, u32 page_num) {
    u32 page_table_index = page_num / 1024;
    u32 page_num_within_table = page_num % 1024;

    if (!page_directory->tables[page_table_index]) {
        // If the page table doesn't exist, we create it.
        page_directory->tables[page_table_index] = kcalloc_aligned(sizeof(Page_Table));
        page_directory->tables_x86_representation[page_table_index] = (u32)(page_directory->tables[page_table_index]) | 0x7; // PRESENT, RW, US
    }

    return &page_directory->tables[page_table_index]->pages[page_num_within_table];
}

// Allocates a frame a given page
static void allocate_frame_to_page(Page_Entry* page_entry) {
    if (page_entry->frame_address) {
        util_panic("trying to allocate frame for page that already has a frame assigned to it.");
    }
    u32 allocd_frame = get_first_frame_available();
    set_frame_used(allocd_frame);
    page_entry->present = 1;
    page_entry->user_mode = 0;  // for now all frames are kernel frames
    page_entry->writable = 1;   // for now all pages are writable
    page_entry->frame_address = allocd_frame;
}

void paging_init() {
    // We allocate a page_directory for the kernel.
    Page_Directory* page_directory = kcalloc_aligned(sizeof(Page_Directory));

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

    // Finally, we enable paging using the kernel page directory that we just created.
    paging_switch_page_directory(page_directory->tables_x86_representation);
}
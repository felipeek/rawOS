#include "gdt.h"
#include "util/util.h"
#include "asm/gdt.h"

#define GDT_NUM_ENTRIES 5

// In C, lower bits come first when struct is declared this way
// e.g.
// struct {
// u8 a : 1
// u8 b : 1
// u8 c : 6
// }
//
// a: 1, b: 0, c: 111111
//
// prints 0xFD
typedef struct __attribute__((packed)) {
	// 2 bytes for Segment Limit[15:00].
	u16 segment_limit1;
	// 2 bytes for Base[15:00]
	u16 base1;
	// 1 byte for Base[23:16]. Should be 0x00.
	u8 base2;

	// 1 bit for 'accessed'. CPU sets this when segment is accessed.
	u8 accessed : 1;
	// 1 bit for 'readable'. 1 if readable, 0 if execute-only. [IF code is 1]
	// 1 if writable, 1 if read-only [IF code is 0]
	u8 readable : 1;
	// 1 bit for 'conforming'. Not conforming means code in a segment with lower privilege may not call code in this segment [IF code is 1]
	// if code is 0, then this means 'expand down'
	u8 conforming : 1;
	// 1 bit for 'code'. Should be 1 for code and 0 for data.
	u8 code : 1;
	// Desciptor Type: (0 = system, 1 = code or data)
	u8 descriptor_type : 1;
	// Descriptor Privilege Level: 00: ring 0 - highest privilege. 11: ring 3 - user mode
	u8 descriptor_privilege_level : 2;
	// Segment Present: Segment is present in memory? (used for virtual memory)
	u8 segment_present : 1;

	// The next 4 bits are the 4 most significant bits of the segment limit (Seg. Limit[19:16]). Our limit is 0xFFFFF
	u8 segment_limit2 : 4;
	// AVL: Available for use by system software. Not using for now, so 0.
	u8 avl : 1;
	// 64-bit Code Segment: 0 for now, stick with 32 bits.
	u8 _64bit_code_segment : 1;
	// Default operation size: 1, because we want 32 bits.
	u8 default_operation_size : 1;
	// Granularity: Limit is multiplied by the page size (4K) ? (1: yes, 0: no)
	u8 granularity : 1;

	// End with Base[31:24]. The base is 0x0
	u8 base3;
} GDT_Entry;

typedef struct __attribute__((packed)) {
	u16 limit;
	u32 base;
} GDT_Descriptor;

static GDT_Entry gdt_entries[GDT_NUM_ENTRIES];
static GDT_Descriptor gdt_descriptor;

void gdt_init() {
	// The first entry of the GDT must be the null descriptor, so we have 8 bytes set to 0x0
	util_memset(&gdt_entries[0], 0, sizeof(GDT_Entry));

	// KERNEL CODE ENTRY
	gdt_entries[1].segment_limit1 = 0xFFFF;
	gdt_entries[1].base1 = 0x0000;
	gdt_entries[1].base2 = 0x00;
	gdt_entries[1].segment_present = 0b1;
	gdt_entries[1].descriptor_privilege_level = 0b00;
	gdt_entries[1].descriptor_type = 0b1;
	gdt_entries[1].code = 0b1;
	gdt_entries[1].conforming = 0b0;
	gdt_entries[1].readable = 0b1;
	gdt_entries[1].accessed = 0b0;
	gdt_entries[1].granularity = 0b1;
	gdt_entries[1].default_operation_size = 0b1;
	gdt_entries[1]._64bit_code_segment = 0b0;
	gdt_entries[1].avl = 0b0;
	gdt_entries[1].segment_limit2 = 0b1111;
	gdt_entries[1].base3 = 0x00;

	// KERNEL DATA ENTRY
	gdt_entries[2].segment_limit1 = 0xFFFF;
	gdt_entries[2].base1 = 0x0000;
	gdt_entries[2].base2 = 0x00;
	gdt_entries[2].segment_present = 0b1;
	gdt_entries[2].descriptor_privilege_level = 0b00;
	gdt_entries[2].descriptor_type = 0b1;
	gdt_entries[2].code = 0b0;
	gdt_entries[2].conforming = 0b0;
	gdt_entries[2].readable = 0b1;
	gdt_entries[2].accessed = 0b0;
	gdt_entries[2].granularity = 0b1;
	gdt_entries[2].default_operation_size = 0b1;
	gdt_entries[2]._64bit_code_segment = 0b0;
	gdt_entries[2].avl = 0b0;
	gdt_entries[2].segment_limit2 = 0b1111;
	gdt_entries[2].base3 = 0x00;

	// USER-MODE CODE ENTRY
	gdt_entries[3].segment_limit1 = 0xFFFF;
	gdt_entries[3].base1 = 0x0000;
	gdt_entries[3].base2 = 0x00;
	gdt_entries[3].segment_present = 0b1;
	gdt_entries[3].descriptor_privilege_level = 0b11;
	gdt_entries[3].descriptor_type = 0b1;
	gdt_entries[3].code = 0b1;
	gdt_entries[3].conforming = 0b0;
	gdt_entries[3].readable = 0b1;
	gdt_entries[3].accessed = 0b0;
	gdt_entries[3].granularity = 0b1;
	gdt_entries[3].default_operation_size = 0b1;
	gdt_entries[3]._64bit_code_segment = 0b0;
	gdt_entries[3].avl = 0b0;
	gdt_entries[3].segment_limit2 = 0b1111;
	gdt_entries[3].base3 = 0x00;

	// USER-MODE DATA ENTRY
	gdt_entries[4].segment_limit1 = 0xFFFF;
	gdt_entries[4].base1 = 0x0000;
	gdt_entries[4].base2 = 0x00;
	gdt_entries[4].segment_present = 0b1;
	gdt_entries[4].descriptor_privilege_level = 0b11;
	gdt_entries[4].descriptor_type = 0b1;
	gdt_entries[4].code = 0b0;
	gdt_entries[4].conforming = 0b0;
	gdt_entries[4].readable = 0b1;
	gdt_entries[4].accessed = 0b0;
	gdt_entries[4].granularity = 0b1;
	gdt_entries[4].default_operation_size = 0b1;
	gdt_entries[4]._64bit_code_segment = 0b0;
	gdt_entries[4].avl = 0b0;
	gdt_entries[4].segment_limit2 = 0b1111;
	gdt_entries[4].base3 = 0x00;

	gdt_descriptor.base = (u32)&gdt_entries;
	gdt_descriptor.limit = sizeof(GDT_Entry) * GDT_NUM_ENTRIES - 1;

	gdt_configure((u32)&gdt_descriptor);
}
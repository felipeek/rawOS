#include "gdt.h"
#include "util/util.h"
#include "asm/gdt.h"

#define GDT_NUM_ENTRIES 3

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
	u16 segment_limit1;
	u16 base1;
	u8 base2;

	u8 accessed : 1;
	u8 readable : 1;
	u8 conforming : 1;
	u8 code : 1;
	u8 descriptor_type : 1;
	u8 descriptor_privilege_level : 2;
	u8 segment_present : 1;

	u8 segment_limit2 : 4;
	u8 avl : 1;
	u8 _64bit_code_segment : 1;
	u8 default_operation_size : 1;
	u8 granularity : 1;

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

	// We start with 2 bytes for Segment Limit[15:00]. Should be 0xFFFF
	gdt_entries[1].segment_limit1 = 0xFFFF;
	// Next we have 2 bytes for Base[15:00]. Should be 0x0000.
	gdt_entries[1].base1 = 0x0000;
	// Next we have 1 byte for Base[23:16]. Should be 0x00.
	gdt_entries[1].base2 = 0x00;
	// Next we have 1 bit for segment present, 2 bits for descriptor privilege level and 1 bit for descriptor type.
	// Segment Present: 1, because the segment is present in memory (used for virtual memory)
	// Descriptor Privilege Level: 00: ring 0 - highest privilege.
	// Desciptor Type: (0 = system, 1 = code or data): 1, because this is the code segment.
	// Next we have 4 bits to define the segment type, which expands in:
	// 1 bit for 'code'. Should be 1 for code and 0 for data. 1 in this case. The meaning of the next 3 fields are different depending on if this is set to code or data.
	// 1 bit for 'conforming'. Not conforming means code in a segment with lower privilege may not call code in this segment. Set to 0.
	// 1 bit for 'readable'. 1 if readable, 0 if execute-only. Set to 1.
	// 1 bit for 'accessed'. CPU sets this when segment is accessed. Set to 0.
	gdt_entries[1].segment_present = 0b1;
	gdt_entries[1].descriptor_privilege_level = 0b00;
	gdt_entries[1].descriptor_type = 0b1;
	gdt_entries[1].code = 0b1;
	gdt_entries[1].conforming = 0b0;
	gdt_entries[1].readable = 0b1;
	gdt_entries[1].accessed = 0b0;
	// Next, we have 4 bits for granularity, default operation size, 64-bit code segment and AVL.
	// Granularity: 1, so our limit is multiplied by the page size (4K)
	// Default operation size: 1, because we want 32 bits.
	// 64-bit Code Segment: 0 for now, stick with 32 bits.
	// AVL: Available for use by system software. Not using for now, so 0.
	// The next 4 bits are the 4 most significant bits of the segment limit (Seg. Limit[19:16]). Our limit is 0xFFFFF
	gdt_entries[1].granularity = 0b1;
	gdt_entries[1].default_operation_size = 0b1;
	gdt_entries[1]._64bit_code_segment = 0b0;
	gdt_entries[1].avl = 0b0;
	gdt_entries[1].segment_limit2 = 0b1111;
	// End with Base[31:24]. The base is 0x0
	gdt_entries[1].base3 = 0x00;

	// We start with 2 bytes for Segment Limit[15:00]. Should be 0xFFFF
	gdt_entries[2].segment_limit1 = 0xFFFF;
	// Next we have 2 bytes for Base[15:00]. Should be 0x0000.
	gdt_entries[2].base1 = 0x0000;
	// Next we have 1 byte for Base[23:16]. Should be 0x00.
	gdt_entries[2].base2 = 0x00;
	// Next we have 1 bit for segment present, 2 bits for descriptor privilege level and 1 bit for descriptor type.
	// Segment Present: 1, because the segment is present in memory (used for virtual memory)
	// Descriptor Privilege Level: 00: ring 0 - highest privilege.
	// Desciptor Type: (0 = system, 1 = code or data): 1, because this is the code segment.
	// Next we have 4 bits to define the segment type, which expands in:
	// 1 bit for 'code'. Should be 1 for code and 0 for data. 0 in this case. The meaning of the next 3 fields are different depending on if this is set to code or data.
	// 1 bit for 'expand down'. Set to 0.
	// 1 bit for 'writable'. 1 if writable, 0 if read-only. Set to 1.
	// 1 bit for 'accessed'. CPU sets this when segment is accessed. Set to 0.
	gdt_entries[2].segment_present = 0b1;
	gdt_entries[2].descriptor_privilege_level = 0b00;
	gdt_entries[2].descriptor_type = 0b1;
	gdt_entries[2].code = 0b0;
	gdt_entries[2].conforming = 0b0;
	gdt_entries[2].readable = 0b1;
	gdt_entries[2].accessed = 0b0;
	// Next, we have 4 bits for granularity, default operation size, 64-bit code segment and AVL.
	// Granularity: 1, so our limit is multiplied by the page size (4K)
	// Default operation size: 1, because we want 32 bits.
	// 64-bit Code Segment: 0 for now, stick with 32 bits.
	// AVL: Available for use by system software. Not using for now, so 0.
	// The next 4 bits are the 4 most significant bits of the segment limit (Seg. Limit[19:16]). Our limit is 0xFFFFF
	gdt_entries[2].granularity = 0b1;
	gdt_entries[2].default_operation_size = 0b1;
	gdt_entries[2]._64bit_code_segment = 0b0;
	gdt_entries[2].avl = 0b0;
	gdt_entries[2].segment_limit2 = 0b1111;
	// End with Base[31:24]. The base is 0x0
	gdt_entries[2].base3 = 0x00;

	gdt_descriptor.base = (u32)&gdt_entries;
	gdt_descriptor.limit = sizeof(GDT_Entry) * GDT_NUM_ENTRIES - 1;

	gdt_configure((u32)&gdt_descriptor);
}
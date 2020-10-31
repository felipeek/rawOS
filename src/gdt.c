#include "gdt.h"
#include "util/util.h"
#include "asm/gdt.h"

#define GDT_NUM_ENTRIES 6

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

// Task State Segment
typedef struct __attribute__((packed)) {
	u32 prev_tss;   // The previous TSS - if we used hardware task switching this would form a linked list.
   
	// ESP (stack pointer) and SS (stack segment) that will be loaded when we switch to ring 0 (kernel mode)
	u32 esp0;
	u32 ss0;

	// ESP1 and SS1 stands for ESP and SS that will be loaded when we switch to ring 1...
	// Same applies to ESP2 and SS2 for ring 2.
	// We don't use ring 1 and ring 2, so they are meaningless
	u32 esp1;
	u32 ss1;
	u32 esp2;
	u32 ss2;

	// Unused
	u32 cr3;
	u32 eip;
	u32 eflags;
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;

	u32 es;         // The value to load into ES when we change to kernel mode.
	u32 cs;         // The value to load into CS when we change to kernel mode.
	u32 ss;         // The value to load into SS when we change to kernel mode.
	u32 ds;         // The value to load into DS when we change to kernel mode.
	u32 fs;         // The value to load into FS when we change to kernel mode.
	u32 gs;         // The value to load into GS when we change to kernel mode.

	// Unused
	u32 ldt;
	u16 trap;
	u16 iomap_base;
} TSS_Entry;

static GDT_Entry gdt_entries[GDT_NUM_ENTRIES];
static GDT_Descriptor gdt_descriptor;
// We have a single TSS entry, and we perform the task management in software
static TSS_Entry tss_entry;

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

	// TSS ENTRY
	u32 tss_base = (u32)&tss_entry;
	u32 tss_limit = tss_base + sizeof(TSS_Entry);
	gdt_entries[5].segment_limit1 = (tss_limit & 0xFFFF);
	gdt_entries[5].base1 = (tss_base & 0xFFFF);
	gdt_entries[5].base2 = ((tss_base >> 16) & 0xFF);
	gdt_entries[5].segment_present = 0b1;
	gdt_entries[5].descriptor_privilege_level = 0b11; // NOT SURE IF 0b11 or 0b00 !
	gdt_entries[5].descriptor_type = 0b0;
	gdt_entries[5].code = 0b1;
	gdt_entries[5].conforming = 0b0;
	gdt_entries[5].readable = 0b0;
	gdt_entries[5].accessed = 0b1;
	gdt_entries[5].granularity = 0b1;				// keeping the same (caution)
	gdt_entries[5].default_operation_size = 0b1;	// keeping the same (caution)
	gdt_entries[5]._64bit_code_segment = 0b0;
	gdt_entries[5].avl = 0b0;
	gdt_entries[5].segment_limit2 = ((tss_limit >> 16) & 0xF);
	gdt_entries[5].base3 = ((tss_base >> 24) & 0xFF);

	util_memset(&tss_entry, 0, sizeof(TSS_Entry));

	// @TODO: make defines for 0x10 (data seg), 0x08 (code seg), and 0x3 (RPL 3)
	tss_entry.ss0 = 0x10;
	tss_entry.esp0 = 0x0;
	// Here we set the cs, ss, ds, es, fs and gs entries in the TSS. These specify what
   	// segments should be loaded when the processor switches to kernel mode. Therefore
   	// they are just our normal kernel code/data segments - 0x08 and 0x10 respectively,
   	// but with the last two bits set, making 0x0b and 0x13. The setting of these bits
   	// sets the RPL (requested privilege level) to 3, meaning that this TSS can be used
   	// to switch to kernel mode from ring 3.
	tss_entry.cs = 0x0b;
	tss_entry.ss = 0x13;
	tss_entry.ds = 0x13;
	tss_entry.es = 0x13;
	tss_entry.fs = 0x13;
	tss_entry.gs = 0x13;

	gdt_descriptor.base = (u32)&gdt_entries;
	gdt_descriptor.limit = sizeof(GDT_Entry) * GDT_NUM_ENTRIES - 1;

	gdt_flush((u32)&gdt_descriptor);
	gdt_tss_flush();
}

void gdt_set_kernel_stack(u32 stack) {
   tss_entry.esp0 = stack;
} 
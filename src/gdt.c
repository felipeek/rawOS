#include "gdt.h"

typedef struct __attribute__((packed)) {
	u16 segment_limit1;
	u16 base1;
	u8 base2;

	u8 segment_present : 1;
	u8 descriptor_privilege_level : 2;
	u8 descriptor_type : 1;
	u8 code : 1;
	u8 conforming : 1;
	u8 readable : 1;
	u8 accessed : 1;

	u8 granularity : 1;
	u8 default_operation_size : 1;
	u8 _64bit_code_segment : 1;
	u8 avl : 1;
	u8 segment_limit2 : 4;

	u8 base3;
} GDT_Entry;

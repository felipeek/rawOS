#include "rawx.h"
#include "util/util.h"
#include "util/printf.h"
#include "paging.h"

static s32 print_header(RawOS_Header* header) {
    if (header->magic[0] == 'R' &&
        header->magic[1] == 'A' &&
        header->magic[2] == 'W' &&
        header->magic[3] == 'X') {
        printf("RawX:\n");
        printf("  Magic:         RAWX\n");
    } else {
        util_panic("Invalid file format, magic RAWX expected\n");
        return -1;
    }
    printf(    "  Version:       %d\n", header->version);
    if (header->flags & RAWX_ARCH_X86) {
        printf("  Architecture:  32bit x86\n");
    } else {
        util_panic("Invalid architecture\n");
        return -1;
    }
    printf("  Load address:  0x%x\n", header->load_address);
    printf("  Entry offset:  0x%x\n", header->entry_point_offset);
    printf("  Stack size:    0x%x\n", header->stack_size);
    printf("  Section count: %d\n", header->section_count);
    printf("\n");

    return 0;
}

static s32 is_printable(u8 c) {
    if (c >= 0 && c <= 31) {
		return 0;
	}
    if (c >= 127) {
		return 0;
	}
    return 1;
}

static s32 print_section_dump(s8* at, s32 length, s32 offset) {
    for (s32 i = 0; i < length; i += 16) {
        printf("  %04X: ", offset + i);
        for(s32 j = 0; j < 16; ++j) {
            if(j + i >= length) {
                printf("   ");
                continue;
            }
            printf("%02X ", (u32)(at[i + j] & 0xff));
        }
        printf(" ");
        for(s32 j = 0; j < 16; ++j) {
            if(j + i >= length) {
                break;
            }
            if(is_printable((u8)at[i + j]))
                printf("%c", at[i+j]);
            else
                printf(".");
        }
        printf("\n");
    }
    printf("\n");
    return 0;
}

s32
print_space(s32 count) {
    for(s32 i = 0; i < count; ++i) printf(" ");
    return 0;
}

//s32
//print_import_section(s8* at, s32 length, s32 offset) {
//    s8* start = at;
//    RawOS_Import_Table* itable = (RawOS_Import_Table*)at;
//    s32 symbol_count = itable->symbol_count;
//    at += sizeof(RawOS_Import_Table);
//
//    RawOS_Import_Address* iaddr = (RawOS_Import_Address*)at;
//
//    printf("  Imports (%d):\n", symbol_count);
//    for(s32 i = 0; i < symbol_count; ++i) {
//        RawOS_Import_Address* imp = iaddr + i;
//        printf("  - %s  ", start + imp->section_symbol_offset);
//        print_space(16 - strlen(start + imp->section_symbol_offset));
//        printf("(module: %s)\n", start + imp->section_lib_offset);
//    }
//
//    return 0;
//}

//s32
//print_sections(s8* base, s8* at, s32 section_count, s32 length) {
//    RawOS_Section* sections = (RawOS_Section*)at;
//
//    printf("Sections:\n");
//    for(s32 i = 0; i < section_count; ++i) {
//        RawOS_Section* sec = sections + i;
//        printf("  Section #%d\n", i + 1);
//        printf("  Name:          %s\n", sec->name);
//        printf("  Size in bytes: %d\n", sec->size_bytes);
//        printf("  Virtual addr:  0x%x\n", sec->virtual_address);
//        printf("  Ptr to data:   0x%x\n", sec->file_ptr_to_data);
//        printf("\n");
//
//        print_section_dump(base + sec->file_ptr_to_data, sec->size_bytes, sec->file_ptr_to_data);
//        if(strncmp(sec->name, ".import", sizeof(".import")) == 0) {
//            print_import_section(base + sec->file_ptr_to_data, sec->size_bytes, sec->file_ptr_to_data);
//        }
//    }
//    return 0;
//}

RawOS_Header* rawx_load(s8* data, s32 length, Page_Directory* process_page_directory) {
    s8* at = data;
    RawOS_Header* header = (RawOS_Header*)at;
    at += sizeof(RawOS_Header);
    if (length < at - data) {
        util_panic("Fatal parse error: end of file within header\n");
    }
    length -= sizeof(RawOS_Header);

    util_assert("rawx parser error: expected RAWX magic", header->magic[0] == 'R' && header->magic[1] == 'A' && header->magic[2] == 'W' && header->magic[3] == 'X');
	util_assert("rawx parser error: expected version 0", header->version == 0);
	util_assert("rawx parser error: expected architecture x86", header->flags & RAWX_ARCH_X86);
	util_assert("rawx parser error: end of file within section table", length >= header->section_count * sizeof(RawOS_Section));

	RawOS_Section* sections = (RawOS_Section*)at;
    for (s32 i = 0; i < header->section_count; ++i) {
        RawOS_Section* sec = sections + i;
		u32 section_address = header->load_address + sec->virtual_address;
		u8* section_data = (u8*)data + sec->file_ptr_to_data;
		util_assert("rawx parser error: section address needs to be 0x1000 aligned", section_address % 0x1000 == 0);

		if (!util_strcmp(sec->name, ".code") || !util_strcmp(sec->name, ".data")) {
			for (u32 i = 0; i < sec->size_bytes; i += 0x1000) {
				u32 target_address = section_address + i;
				u32 page_num = target_address / 0x1000;
				u32 data_chunk_size = MIN(0x1000, sec->size_bytes - i);
				paging_create_page_with_any_frame(process_page_directory, page_num, 1);
				util_memcpy((void*)target_address, section_data, data_chunk_size);
			}
		}
    }

    return header;
}
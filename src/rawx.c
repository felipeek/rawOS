#include "rawx.h"
#include "util/util.h"
#include "util/printf.h"
#include "paging.h"
#include "syscall.h"

#define RAWX_STACK_ADDRESS 0xC0000000
#define RAWX_STACK_ADDRESS_MAX_RESERVED_PAGES 2048
#define RAWX_IMPORT_DATA_MAX_RESERVED_PAGES 2048

static s32 print_header(RawX_Header* header) {
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
//    RawX_Import_Table* itable = (RawX_Import_Table*)at;
//    s32 symbol_count = itable->symbol_count;
//    at += sizeof(RawX_Import_Table);
//
//    RawX_Import_Address* iaddr = (RawX_Import_Address*)at;
//
//    printf("  Imports (%d):\n", symbol_count);
//    for(s32 i = 0; i < symbol_count; ++i) {
//        RawX_Import_Address* imp = iaddr + i;
//        printf("  - %s  ", start + imp->section_symbol_offset);
//        print_space(16 - strlen(start + imp->section_symbol_offset));
//        printf("(module: %s)\n", start + imp->section_lib_offset);
//    }
//
//    return 0;
//}

//s32
//print_sections(s8* base, s8* at, s32 section_count, s32 length) {
//    RawX_Section* sections = (RawX_Section*)at;
//
//    printf("Sections:\n");
//    for(s32 i = 0; i < section_count; ++i) {
//        RawX_Section* sec = sections + i;
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

RawX_Load_Information rawx_load(s8* data, s32 length, Page_Directory* process_page_directory, s32 create_stack) {
    s8* at = data;
    RawX_Header* header = (RawX_Header*)at;
    at += sizeof(RawX_Header);
    if (length < at - data) {
        util_panic("Fatal parse error: end of file within header\n");
    }
    length -= sizeof(RawX_Header);

    util_assert("rawx parser error: expected RAWX magic", header->magic[0] == 'R' && header->magic[1] == 'A' && header->magic[2] == 'W' && header->magic[3] == 'X');
	util_assert("rawx parser error: expected version 0", header->version == 0);
	util_assert("rawx parser error: expected architecture x86", header->flags & RAWX_ARCH_X86);
	util_assert("rawx parser error: end of file within section table", length >= header->section_count * sizeof(RawX_Section));
	util_assert("rawx parser error: load address must be greater than 1gb", header->load_address >= 1024 * 1024 * 1024);

	RawX_Load_Information rli;

	RawX_Section* sections = (RawX_Section*)at;
    for (s32 i = 0; i < header->section_count; ++i) {
        RawX_Section* sec = sections + i;
		u32 section_address = header->load_address + sec->virtual_address;
		u8* section_data = (u8*)data + sec->file_ptr_to_data;
		util_assert("rawx parser error: section address needs to be 0x1000 aligned", section_address % 0x1000 == 0);
		util_assert("rawx parser error: section address + size is too high", section_address + sec->size_bytes <
			RAWX_STACK_ADDRESS - RAWX_STACK_ADDRESS_MAX_RESERVED_PAGES * 0x1000 - RAWX_IMPORT_DATA_MAX_RESERVED_PAGES * 0x1000);

		if (!util_strcmp(sec->name, ".code")) {
			rli.code_address = section_address;
			for (u32 i = 0; i < sec->size_bytes; i += 0x1000) {
				u32 target_address = section_address + i;
				u32 page_num = target_address / 0x1000;
				u32 data_chunk_size = MIN(0x1000, sec->size_bytes - i);
				paging_create_page_with_any_frame(process_page_directory, page_num, 1);
				util_memcpy((void*)target_address, section_data, data_chunk_size);
			}
		} else if (!util_strcmp(sec->name, ".data")) {
			rli.data_address = section_address;
			for (u32 i = 0; i < sec->size_bytes; i += 0x1000) {
				u32 target_address = section_address + i;
				u32 page_num = target_address / 0x1000;
				u32 data_chunk_size = MIN(0x1000, sec->size_bytes - i);
				paging_create_page_with_any_frame(process_page_directory, page_num, 1);
				util_memcpy((void*)target_address, section_data, data_chunk_size);
			}
		} else if (!util_strcmp(sec->name, ".import")) {
			s8* start = data + sec->file_ptr_to_data;
			at = start;
			RawX_Import_Table* itable = (RawX_Import_Table*)at;
			s32 symbol_count = itable->symbol_count;
			at += sizeof(RawX_Import_Table);

			RawX_Import_Address* iaddr = (RawX_Import_Address*)at;

			// @TODO: this can be put immediately below the stack, not below the MAX reserved space for the stack
			u32 page_addr = RAWX_STACK_ADDRESS - RAWX_STACK_ADDRESS_MAX_RESERVED_PAGES * 0x1000
				- RAWX_IMPORT_DATA_MAX_RESERVED_PAGES * 0x1000;
			// @TODO: For now, let's alloc a single page for imports, for simplicity.
			// In the future, we can alloc the next pages, starting at page_addr
			paging_create_page_with_any_frame(process_page_directory, page_addr / 0x1000, 1);

			u32 current_addr = page_addr;
			for (s32 i = 0; i < symbol_count; ++i) {
				RawX_Import_Address* imp = iaddr + i;
				s8* symbol_name = start + imp->section_symbol_offset;
				s8* lib_name = start + imp->section_lib_offset;
				u32* call_address = &imp->call_address;
				util_assert("rawx parser error: import has unknown lib.", !util_strcmp(lib_name, "kernel"));

				Syscall_Stub_Information ssi;
				util_assert("rawx parser error: import has unknown symbol.", !syscall_stub_get(symbol_name, &ssi));

				util_assert("rawx_parser_error: more than one page needed for imports! go fix it!",
					current_addr + ssi.syscall_stub_size < page_addr + 0x1000);
				util_memcpy((void*)current_addr, (void*)ssi.syscall_stub_address, ssi.syscall_stub_size);
				// set the call address!
				*call_address = current_addr;
				current_addr += ssi.syscall_stub_size;
			}

			// finish by copying the section, since all call addresses are set now.
			for (u32 i = 0; i < sec->size_bytes; i += 0x1000) {
				u32 target_address = section_address + i;
				u32 page_num = target_address / 0x1000;
				u32 data_chunk_size = MIN(0x1000, sec->size_bytes - i);
				paging_create_page_with_any_frame(process_page_directory, page_num, 1);
				util_memcpy((void*)target_address, section_data, data_chunk_size);
			}
		}
    }

	if (create_stack) {
		util_assert("stack size must be greater than 0", header->stack_size > 0);

		u32 stack_pages = header->stack_size / 0x1000 + 1;
		for (u32 i = 0; i < stack_pages; ++i) {
			u32 page_num = (RAWX_STACK_ADDRESS / 0x1000) - i;
			paging_create_page_with_any_frame(process_page_directory, page_num, 1);
		}
	}

	rli.entrypoint = header->load_address + header->entry_point_offset;

    return rli;
}
#include "rawx.h"
#include "util/util.h"
#include "util/printf.h"
#include "paging.h"
#include "syscall.h"
#include "process.h"

#define RAWX_LOAD_ADDRESS_MINIMUM (1024 * 1024 * 1024)
#define RAWX_SECTION_ADDRESS_MAXIMUM (RAWX_STACK_ADDRESS - RAWX_STACK_ADDRESS_MAX_RESERVED_PAGES * 0x1000 - RAWX_IMPORT_DATA_MAX_RESERVED_PAGES * 0x1000)
#define RAWX_KERNEL_LIB_NAME "kernel"

RawX_Load_Information rawx_load(s8* data, s32 length, Page_Directory* process_page_directory, s32 create_stack, s32 create_kernel_stack) {
    s8* at = data;
    RawX_Header* header = (RawX_Header*)at;
    at += sizeof(RawX_Header);
    if (length < at - data) {
        util_panic("Fatal parse error: end of file within header\n");
    }
    length -= sizeof(RawX_Header);

    util_assert(header->magic[0] == 'R' && header->magic[1] == 'A' && header->magic[2] == 'W' && header->magic[3] == 'X',
		"Error loading RawX: RAW magic not present");
	util_assert(header->version == 0, "Error loading RawX: Version %u not supported", header->version);
	util_assert(header->flags & RAWX_ARCH_X86, "Error loading RawX: x86 architecture is mandatory");
	util_assert(length >= header->section_count * sizeof(RawX_Section),
		"Error loading RawX: End of file within section table");
	util_assert(header->load_address >= RAWX_LOAD_ADDRESS_MINIMUM,
		"Error loading RawX: Load address is too short. Needs to be at least 0x%x, but got 0x%x.",
		RAWX_LOAD_ADDRESS_MINIMUM, header->load_address);

	RawX_Load_Information rli;

	RawX_Section* sections = (RawX_Section*)at;
    for (s32 i = 0; i < header->section_count; ++i) {
        RawX_Section* sec = sections + i;
		u32 section_address = header->load_address + sec->virtual_address;
		u8* section_data = (u8*)data + sec->file_ptr_to_data;
		util_assert(section_address % 0x1000 == 0,
			"Error loading RawX: section address needs to be 0x1000 aligned, but got 0x%x.", section_address);
		util_assert(section_address + sec->size_bytes < RAWX_SECTION_ADDRESS_MAXIMUM,
			"Error loading RawX: (section address + size in bytes) is too high! Got 0x%x but can't be greater than 0x%x.",
			section_address + sec->size_bytes, RAWX_SECTION_ADDRESS_MAXIMUM);

		if (!util_strcmp(sec->name, ".code")) {
			rli.code_address = section_address;
			for (u32 i = 0; i < sec->size_bytes; i += 0x1000) {
				u32 target_address = section_address + i;
				u32 page_num = target_address / 0x1000;
				u32 data_chunk_size = MIN(0x1000, sec->size_bytes - i);
				paging_create_process_page_with_any_frame(process_page_directory, page_num, 1);
				util_memcpy((void*)target_address, section_data, data_chunk_size);
			}
		} else if (!util_strcmp(sec->name, ".data")) {
			rli.data_address = section_address;
			for (u32 i = 0; i < sec->size_bytes; i += 0x1000) {
				u32 target_address = section_address + i;
				u32 page_num = target_address / 0x1000;
				u32 data_chunk_size = MIN(0x1000, sec->size_bytes - i);
				paging_create_process_page_with_any_frame(process_page_directory, page_num, 1);
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
			paging_create_process_page_with_any_frame(process_page_directory, page_addr / 0x1000, 1);

			u32 current_addr = page_addr;
			for (s32 i = 0; i < symbol_count; ++i) {
				RawX_Import_Address* imp = iaddr + i;
				s8* symbol_name = start + imp->section_symbol_offset;
				s8* lib_name = start + imp->section_lib_offset;
				u32* call_address = &imp->call_address;
				printf("rawx: found symbol %s:%s\n", symbol_name, lib_name);
				util_assert(!util_strcmp(lib_name, RAWX_KERNEL_LIB_NAME), "Error loading RawX: import has unknown lib (%s).", lib_name);

				Syscall_Stub_Information ssi;
				util_assert(!syscall_stub_get(symbol_name, &ssi), "Error loading RawX: import has unknown symbol (%s).", symbol_name);

				util_assert(current_addr + ssi.syscall_stub_size < page_addr + 0x1000,
					"Error loading RawX: more than one page needed for imports! go fix it!");
				util_memcpy((void*)current_addr, (void*)ssi.syscall_stub_address, ssi.syscall_stub_size);
				// set the call address!
				*call_address = current_addr;
				current_addr += ssi.syscall_stub_size;
				printf("rawx: call address 0x%x set for syscall %s.\n", *call_address, symbol_name);
			}

			// finish by copying the section, since all call addresses are set now.
			for (u32 i = 0; i < sec->size_bytes; i += 0x1000) {
				u32 target_address = section_address + i;
				u32 page_num = target_address / 0x1000;
				u32 data_chunk_size = MIN(0x1000, sec->size_bytes - i);
				paging_create_process_page_with_any_frame(process_page_directory, page_num, 1);
				util_memcpy((void*)target_address, section_data, data_chunk_size);
			}
		}
    }

	if (create_stack) {
		util_assert(header->stack_size > 0, "Error loading RawX: stack size must be greater than 0 (got 0x%x)", header->stack_size);
		util_assert(header->stack_size % 0x1000 == 0, "Error loading RawX: stack size must be 0x1000 aligned (got 0x%x)", header->stack_size);

		u32 stack_pages = header->stack_size / 0x1000;
		util_assert(stack_pages <= RAWX_STACK_ADDRESS_MAX_RESERVED_PAGES,
			"Error loading RawX: stack is too big! Got %u needed pages, but max is %u!", stack_pages, RAWX_STACK_ADDRESS_MAX_RESERVED_PAGES);
		for (u32 i = 0; i < stack_pages; ++i) {
			u32 page_num = (RAWX_STACK_ADDRESS / 0x1000) - i;
			paging_create_process_page_with_any_frame(process_page_directory, page_num, 1);
		}

		rli.stack_address = RAWX_STACK_ADDRESS;
	}

	if (create_kernel_stack) {
		for (u32 i = 0; i < KERNEL_STACK_RESERVED_PAGES_IN_PROCESS_ADDRESS_SPACE; ++i) {
			u32 page_num = (KERNEL_STACK_ADDRESS_IN_PROCESS_ADDRESS_SPACE / 0x1000) - i;
			paging_create_process_page_with_any_frame(process_page_directory, page_num, 0);
		}
	}

	rli.entrypoint = header->load_address + header->entry_point_offset;

    return rli;
}
#ifndef RAW_OS_RAWX_H
#define RAW_OS_RAWX_H
#include "common.h"
#include "paging.h"

#define RAWX_ARCH_X86 0x1
#define RAWX_VERSION 0

#define RAWX_STACK_ADDRESS 0xC0000000
#define RAWX_STACK_ADDRESS_MAX_RESERVED_PAGES 2048
#define RAWX_IMPORT_DATA_MAX_RESERVED_PAGES 2048

typedef struct {
    u8  magic[4];        // RAWX
    u16 version;
    u32 flags;
    u32 load_address;
    u32 entry_point_offset;
    u32 stack_size;
    s32 section_count;
} RawX_Header;

typedef struct {
    u8  name[8];            // .code .data .import
    u32 size_bytes;
    u32 virtual_address;    // offset from load_address, must be aligned to 0x1000
    u32 file_ptr_to_data;   // offset within the file to the section data
} RawX_Section;

// Next in the file is the data for all the sections

// .code is just raw code

// .data is just raw data

// .import
typedef struct {
    u32 section_symbol_offset;  // offset from the beginning of the section where the symbol name is located
    u32 section_lib_offset;     // offset from the beginning of the section where the library name is located
    u32 call_address;           // address to the syscall to be filled by the loader
} RawX_Import_Address;

typedef struct {
    u32 symbol_count;
    RawX_Import_Address import_addresses[0];
} RawX_Import_Table;

// Next the symbol names and library names can be located anywhere
// the only thing that needs to be done is fill the offsets in the import
// addresses properly.
// char* symbol_name;
// char* symbol_library;

typedef struct {
	u32 code_address;
	u32 data_address;
	u32 stack_address;
	u32 entrypoint;
} RawX_Load_Information;

RawX_Load_Information rawx_load(s8* data, s32 length, Page_Directory* process_page_directory, s32 create_stack, s32 create_kernel_stack);
#endif
#ifndef RAW_OS_ASM_PROCESS_H
#define RAW_OS_ASM_PROCESS_H
#include "../common.h"
void process_switch_context(u32 eip, u32 esp, u32 ebp, u32 page_directory_x86_tables_frame_addr);
#endif
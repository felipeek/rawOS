#ifndef RAW_OS_ASM_PROCESS_H
#define RAW_OS_ASM_PROCESS_H
#include "../common.h"
extern u32 process_switch_context_magic_return_value;
void process_switch_context(u32 eip, u32 esp, u32 ebp, u32 page_directory_x86_tables_frame_addr);
void process_switch_to_user_mode_set_stack_and_jmp_addr(u32 esp, u32 addr);
#endif
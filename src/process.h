#ifndef RAW_OS_PROCESS_H
#define RAW_OS_PROCESS_H
#include "common.h"
#define KERNEL_STACK_ADDRESS_IN_PROCESS_ADDRESS_SPACE 0xF0000000
#define KERNEL_STACK_RESERVED_PAGES_IN_PROCESS_ADDRESS_SPACE 2048
void process_init();
s32 process_fork();
void process_switch();
void process_execve(const s8* image_path);
void process_link_kernel_table_to_all_address_spaces(u32 page_table_virtual_address, u32 page_table_index, u32 page_table_x86_representation);
#endif
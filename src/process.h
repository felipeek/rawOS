#ifndef RAW_OS_PROCESS_H
#define RAW_OS_PROCESS_H
#include "common.h"
void process_init();
s32 process_fork();
void process_switch();
void process_link_kernel_table_to_all_address_spaces(u32 page_table_virtual_address, u32 page_table_index, u32 page_table_x86_representation);
#endif
#ifndef RAW_OS_ASM_PAGING_H
#define RAW_OS_ASM_PAGING_H
#include "../common.h"
#include "../paging.h"
void paging_switch_page_directory(u32* page_directory);
u32 paging_get_faulting_address();
#endif
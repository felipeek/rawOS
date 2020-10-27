#ifndef RAW_OS_PAGING_H
#define RAW_OS_PAGING_H
#include "common.h"

void paging_init();
u32 paging_create_kernel_page_with_any_frame(u32 page_num);
void test_clone();

#endif
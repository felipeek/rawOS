#ifndef RAW_OS_PAGING_H
#define RAW_OS_PAGING_H
#include "common.h"

void paging_init();
void paging_create_page_and_allocate_frame(u32 page_num);

#endif
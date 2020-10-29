#ifndef RAW_OS_ASM_GDT_H
#define RAW_OS_ASM_GDT_H
#include "../common.h"
void gdt_configure(u32 gdt_ptr);
#endif
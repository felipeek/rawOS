#ifndef RAW_OS_ASM_GDT_H
#define RAW_OS_ASM_GDT_H
#include "../common.h"
void gdt_flush(u32 gdt_ptr);
void gdt_tss_flush();
#endif
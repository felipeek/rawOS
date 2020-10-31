#ifndef RAW_OS_GDT_H
#define RAW_OS_GDT_H
#include "common.h"
void gdt_init();
void gdt_set_kernel_stack(u32 stack);
#endif
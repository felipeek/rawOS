#ifndef RAW_OS_KMALLOC_H
#define RAW_OS_KMALLOC_H
#include "common.h"
void* kmalloc(u32 size);
void* kmalloc_aligned(u32 size);
void* kcalloc(u32 size);
void* kcalloc_aligned(u32 size);
#endif
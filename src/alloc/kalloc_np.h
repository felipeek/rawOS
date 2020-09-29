#ifndef RAW_OS_ALLOC_KALLOC_NP_H
#define RAW_OS_ALLOC_KALLOC_NP_H
#include "../common.h"

// Util functions to allocate memory space for the kernel when paging is not enabled.
void* kmalloc_np(u32 size);
void* kmalloc_np_aligned(u32 size);
void* kcalloc_np(u32 size);
void* kcalloc_np_aligned(u32 size);
#endif
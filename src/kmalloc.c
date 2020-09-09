#include "kmalloc.h"
#include "util.h"

// The end section is defined in link.ld
// This global variable basically points to the end of memory reserved to the kernel.
extern u32 end;
u32 kmalloc_addr = (u32)&end;

void* kmalloc(u32 size) {
    void* addr = (void*)kmalloc_addr;
    kmalloc_addr += size;
    return addr;
}

void* kmalloc_aligned(u32 size) {
    if (size & 0xFFFFF000) {
        kmalloc_addr &= 0xFFFFF000;
        kmalloc_addr += 0x1000;
    }
    void* addr = (void*)kmalloc_addr;
    kmalloc_addr += size;
    return addr;
}

void* kcalloc(u32 size) {
    void* ptr = kmalloc(size);
    util_memset(ptr, 0, size);
    return ptr;
}

void* kcalloc_aligned(u32 size) {
    void* ptr = kmalloc_aligned(size);
    util_memset(ptr, 0, size);
    return ptr;
}
#include "kalloc_np.h"
#include "../util/util.h"

// The end section is defined in link.ld
// This global variable basically points to the end of memory reserved to the kernel.
extern u32 end;
u32 kmalloc_addr = (u32)&end;

// No-paging kernel allocation (_np functions)
void* kmalloc_np(u32 size) {
	void* addr = (void*)kmalloc_addr;
	kmalloc_addr += size;
	return addr;
}

void* kmalloc_np_aligned(u32 size) {
	if (size & 0xFFFFF000) {
		kmalloc_addr &= 0xFFFFF000;
		kmalloc_addr += 0x1000;
	}
	void* addr = (void*)kmalloc_addr;
	kmalloc_addr += size;
	return addr;
}

void* kcalloc_np(u32 size) {
	void* ptr = kmalloc_np(size);
	util_memset(ptr, 0, size);
	return ptr;
}

void* kcalloc_np_aligned(u32 size) {
	void* ptr = kmalloc_np_aligned(size);
	util_memset(ptr, 0, size);
	return ptr;
}
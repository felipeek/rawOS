#ifndef RAW_OS_SYSCALL_H
#define RAW_OS_SYSCALL_H
#include "common.h"
typedef struct {
	u32 syscall_stub_address;
	u32 syscall_stub_size;
} Syscall_Stub_Information;

void syscall_init();
s32 syscall_stub_get(const s8* syscall_name, Syscall_Stub_Information* ssi);
#endif
#ifndef RAW_OS_ASM_SYSCALL_STUBS_H
#define RAW_OS_ASM_SYSCALL_STUBS_H
#include "../common.h"
void syscall_print_stub(s8* str);
extern u32 syscall_print_stub_size;
#endif
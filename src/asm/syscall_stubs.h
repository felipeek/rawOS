#ifndef RAW_OS_ASM_SYSCALL_STUBS_H
#define RAW_OS_ASM_SYSCALL_STUBS_H
#include "../common.h"
void syscall_print_stub();
extern u32 syscall_print_stub_size;
void syscall_exit_stub();
extern u32 syscall_exit_stub_size;
void syscall_pos_cursor_stub();
extern u32 syscall_pos_cursor_stub_size;
void syscall_clear_screen_stub();
extern u32 syscall_clear_screen_stub_size;
void syscall_execve_stub();
extern u32 syscall_execve_stub_size;
void syscall_fork_stub();
extern u32 syscall_fork_stub_size;
void syscall_open_stub();
extern u32 syscall_open_stub_size;
void syscall_read_stub();
extern u32 syscall_read_stub_size;
void syscall_write_stub();
extern u32 syscall_write_stub_size;
void syscall_close_stub();
extern u32 syscall_close_stub_size;
#endif
#ifndef RAW_OS_PROCESS_H
#define RAW_OS_PROCESS_H
#include "common.h"
#include "fs/vfs.h"

#define KERNEL_STACK_ADDRESS_IN_PROCESS_ADDRESS_SPACE 0xF0000000
#define KERNEL_STACK_RESERVED_PAGES_IN_PROCESS_ADDRESS_SPACE 2048

typedef enum {
	PROCESS_RUNNING,
	PROCESS_READY,
	PROCESS_BLOCKED
} Process_State;

void process_init();
s32 process_fork();
void process_switch(Process_State new_state);
s32 process_execve(const s8* image_path);
void process_exit(u32 ret);
u32 process_getpid();
void process_link_kernel_table_to_all_address_spaces(u32 page_table_virtual_address, u32 page_table_index, u32 page_table_x86_representation);

void process_unblock(u32 pid);
void process_block_current();

s32 process_add_fd_to_active_process(Vfs_Node* node);
void process_remove_fd_from_active_process(s32 fd);
Vfs_Node* process_get_node_of_fd_of_active_process(s32 fd);
#endif
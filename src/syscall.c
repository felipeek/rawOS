#include "syscall.h"
#include "interrupt.h"
#include "util/printf.h"
#include "hash_map.h"
#include "asm/syscall_stubs.h"
#include "process.h"
#include "screen.h"
#include "fs/util.h"
#include "fs/vfs.h"

Hash_Map syscall_stubs;

static const s8 PRINT_SYSCALL_NAME[] = "print";
static const s8 FORK_SYSCALL_NAME[] = "fork";
static const s8 POS_CURSOR_SYSCALL_NAME[] = "pos_cursor";
static const s8 CLEAR_SCREEN_SYSCALL_NAME[] = "clear_screen";
static const s8 EXECVE_SYSCALL_NAME[] = "execve";
static const s8 EXIT_SYSCALL_NAME[] = "exit";
static const s8 OPEN_SYSCALL_NAME[] = "open";
static const s8 READ_SYSCALL_NAME[] = "read";
static const s8 WRITE_SYSCALL_NAME[] = "write";
static const s8 CLOSE_SYSCALL_NAME[] = "close";

// Compares two keys. Needs to return 1 if the keys are equal, 0 otherwise.
static s32 syscall_stub_name_compare(const void* _key1, const void* _key2) {
	s8* key1 = *(s8**)_key1;
	s8* key2 = *(s8**)_key2;
	return !strcmp(key1, key2);
}

// Calculates the hash of the key.
static u32 syscall_stub_name_hash(const void *key) {
	const s8* str = *(const s8**)key;
	u32 hash = 5381;
	s32 c;
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

static void syscall_handler(Interrupt_Handler_Args* args) {
	switch(args->eax) {
		case 0: {
			// print syscall
			printf("%s", args->ebx);
		} break;
		case 1: {
			// exit syscall
			process_exit(args->ebx);
		} break;
		case 2: {
			// pos_cursor syscall
			screen_pos_cursor(args->ebx, args->ecx);
		} break;
		case 3: {
			// clear_screen syscall
			screen_clear();
		} break;
		case 4: {
			// exceve syscall
			process_execve((s8*)args->ebx);
		} break;
		case 5: {
			// fork syscall
			args->eax = process_fork();
		} break;
		case 6: {
			// open syscall
			const s8* path = (const s8*)args->ebx;
			Vfs_Node* node = fs_util_get_node_by_path(path);
			if (node) {
				vfs_open(node, 0);
				args->eax = process_add_fd_to_active_process(node);
			} else {
				args->eax = -1;
			}
		} break;
		case 7: {
			// read syscall
			s32 fd = (s32)args->ebx;
			u8* buf = (u8*)args->ecx;
			u32 count = args->edx;
			Vfs_Node* node = process_get_node_of_fd_of_active_process(fd);
			if (node) {
				args->eax = vfs_read(node, 0, count, buf);
			} else {
				args->eax = -1;
			}
		} break;
		case 8: {
			// write syscall
			s32 fd = (s32)args->ebx;
			u8* buf = (u8*)args->ecx;
			u32 count = args->edx;
			Vfs_Node* node = process_get_node_of_fd_of_active_process(fd);
			if (node) {
				args->eax = vfs_write(node, 0, count, buf);
			} else {
				args->eax = -1;
			}
		} break;
		case 9: {
			// close syscall
			s32 fd = (s32)args->ebx;
			Vfs_Node* node = process_get_node_of_fd_of_active_process(fd);
			if (node) {
				vfs_close(node);
				args->eax = 0;
			} else {
				args->eax = -1;
			}
		} break;
	}
}

s32 syscall_stub_get(const s8* syscall_name, Syscall_Stub_Information* ssi) {
	return hash_map_get(&syscall_stubs, &syscall_name, ssi);
}

void syscall_init() {
	hash_map_create(&syscall_stubs, 1024, sizeof(s8*), sizeof(Syscall_Stub_Information), syscall_stub_name_compare, syscall_stub_name_hash);
	
	const s8* syscall_name;
	Syscall_Stub_Information ssi;
	ssi.syscall_stub_address = (u32)syscall_print_stub;
	ssi.syscall_stub_size = syscall_print_stub_size;
	syscall_name = PRINT_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_exit_stub;
	ssi.syscall_stub_size = syscall_exit_stub_size;
	syscall_name = EXIT_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_pos_cursor_stub;
	ssi.syscall_stub_size = syscall_pos_cursor_stub_size;
	syscall_name = POS_CURSOR_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_clear_screen_stub;
	ssi.syscall_stub_size = syscall_clear_screen_stub_size;
	syscall_name = CLEAR_SCREEN_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_execve_stub;
	ssi.syscall_stub_size = syscall_execve_stub_size;
	syscall_name = EXECVE_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_fork_stub;
	ssi.syscall_stub_size = syscall_fork_stub_size;
	syscall_name = FORK_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_open_stub;
	ssi.syscall_stub_size = syscall_open_stub_size;
	syscall_name = OPEN_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_read_stub;
	ssi.syscall_stub_size = syscall_read_stub_size;
	syscall_name = READ_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_write_stub;
	ssi.syscall_stub_size = syscall_write_stub_size;
	syscall_name = WRITE_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	ssi.syscall_stub_address = (u32)syscall_close_stub;
	ssi.syscall_stub_size = syscall_close_stub_size;
	syscall_name = CLOSE_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	interrupt_register_handler(syscall_handler, ISR128);
}

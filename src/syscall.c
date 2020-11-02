#include "syscall.h"
#include "interrupt.h"
#include "util/printf.h"
#include "hash_map.h"
#include "asm/syscall_stubs.h"
#include "process.h"
#include "screen.h"

Hash_Map syscall_stubs;

static const s8 PRINT_SYSCALL_NAME[] = "print";
static const s8 FORK_SYSCALL_NAME[] = "fork";
static const s8 POS_CURSOR_SYSCALL_NAME[] = "pos_cursor";
static const s8 CLEAR_SCREEN_SYSCALL_NAME[] = "clear_screen";
static const s8 EXECVE_SYSCALL_NAME[] = "execve";
static const s8 EXIT_SYSCALL_NAME[] = "exit";

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
			// fork syscall
			args->eax = process_fork();
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
			// exit syscall
			process_exit(args->ebx);
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
	ssi.syscall_stub_address = (u32)syscall_fork_stub;
	ssi.syscall_stub_size = syscall_fork_stub_size;
	syscall_name = FORK_SYSCALL_NAME;
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
	ssi.syscall_stub_address = (u32)syscall_exit_stub;
	ssi.syscall_stub_size = syscall_exit_stub_size;
	syscall_name = EXIT_SYSCALL_NAME;
	hash_map_put(&syscall_stubs, &syscall_name, &ssi);
	interrupt_register_handler(syscall_handler, ISR128);
}

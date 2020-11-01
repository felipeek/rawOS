#define HASH_MAP_IMPLEMENT
#include "hash_map.h"
#include "common.h"
#include "screen.h"
#include "interrupt.h"
#include "timer.h"
#include "keyboard.h"
#include "paging.h"
#include "alloc/kalloc.h"
#include "alloc/kalloc_test.h"
#include "fs/vfs.h"
#include "fs/initrd.h"
#include "util/printf.h"
#include "util/util.h"
#include "process.h"
#include "gdt.h"
#include "syscall.h"
#include "asm/process.h"
#include "rawx.h"

void print_logo() {
	s8 logo[] =
"       -----------     ------    ---      ---   --------   ------------  \n"
"       ***********    ********   ***  **  ***  **********  ************  \n"
"       ----    ---   ----------  ---  --  --- ----    ---- ----          \n"
"       *********    ****    **** ***  **  *** ***      *** ************  \n"
"       ---------    ------------ ---  --  --- ---      --- ------------  \n"
"       ****  ****   ************ ************ ****    ****        *****  \n"
"       ----   ----  ----    ----  ----------   ----------  ------------  \n"
"       ****    **** ****    ****   ********     ********   ************  \n"
;
	printf(logo);
	printf("\n\nWelcome to rawOS!\n");
}

void read_folder(Vfs_Node* folder_node) {
	Vfs_Dirent dirent;
	s32 index = 0;
	while (!vfs_readdir(folder_node, index++, &dirent)) {
		printf("Found file: %s\n", dirent.name);

		Vfs_Node* node = vfs_lookup(folder_node, dirent.name);
		util_assert("node is null, but it can't be!", node != 0);

		printf("Content: ");

		const u32 buffer_size = 64;
		u8 buffer[buffer_size];
		s32 offset = 0;
		s32 read;

		while (read = vfs_read(node, offset, buffer_size - 1, buffer)) {
			offset += read;
			buffer[read] = 0;
			printf("%s", buffer);
		}

		printf("\n");
	}
}

void main() {
	gdt_init();
	screen_init();
	screen_clear();
	print_logo();

	//screen_print("ç\n");
	//While(1);

	timer_init();
	paging_init();
	kalloc_init(1);
	interrupt_init();
	keyboard_init();
	syscall_init();
	vfs_root = initrd_init();

	printf("Kernel initialization completed.\n");
	printf("Starting processes and switching to user-mode...\n");

	process_init();

	//printf("hello user-mode world!");
	//asm volatile("int $0x80");
	//while(1);

	//process_enter_user_mode();
	//while(1) { 
	//	printf("asdN\n");
	//	asm volatile("int $0x80");
	//}

	//process_init();
	//u32 pid = process_fork();
	//if (pid == 0) {
	//	// child
	//	while (1) {
	//		printf("I am the child!\n");
	//	}
	//} else {
	//	// parent
	//	pid = process_fork();
	//	if (pid == 0) {
	//		// child
	//		while (1) {
	//			u8 stack_test[16] = "hello";
	//			printf("I am the second child (%s)!\n", stack_test);
	//		}
	//	} else {
	//		while (1) {
	//			s32 stack_test = 3;
	//			printf("I am the parent (%d)!\n", stack_test);
	//		}
	//	}
	//}

	//Vfs_Node* fs = initrd_init();
	//Vfs_Node* rawx_node = vfs_lookup(fs, "out1.rawx");
	//printf("rawx_node: %u\n", rawx_node);
	//u8* buffer = kalloc_alloc(1024 * 1024);
	//s32 len = vfs_read(rawx_node, 0, 1024 * 1024, buffer);
	//rawx_parse(buffer, len);
	
	//read_folder(fs);

	//kalloc_heap_print(&heap);

	// Force page fault
	//u8* invalid_addr = (u8*)0xA0000000;
	//u8 v = *invalid_addr;
}
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
		Vfs_Node* node = vfs_lookup(folder_node, dirent.name);
		assert(node != 0, "Lookup of file %s returned null node!", dirent.name);

		if (node->flags & VFS_DIRECTORY) {
			printf("Found directory: %s\n", dirent.name);
			read_folder(node);
		} else {
			const u32 buffer_size = 10;
			u8 buffer[buffer_size];
			s32 offset = 0;
			s32 read;

			read = vfs_read(node, offset, buffer_size - 1, buffer);
			buffer[read] = 0;
			printf("Found file: %s (%s)\n", dirent.name, buffer);
		}

	}
}

void main() {
	gdt_init();
	screen_init();
	screen_clear();
	print_logo();

	timer_init();
	paging_init();
	kalloc_init(1);
	interrupt_init();
	keyboard_init();
	syscall_init();
	vfs_init();

	read_folder(vfs_root);

	printf("Kernel initialization completed.\n");
	printf("Starting processes and switching to user-mode...\n");

	process_init();
}
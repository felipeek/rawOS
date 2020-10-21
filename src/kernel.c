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
	timer_init();
	paging_init();
	screen_init();
	screen_clear();
	kalloc_init(1);

	print_logo();

	interrupt_init();
	keyboard_init();
	printf("Works like a charm :)\n");

	Vfs_Node* fs = initrd_init();
	read_folder(fs);

	//kalloc_heap_print(&heap);

	// Force page fault
	//u8* invalid_addr = (u8*)0xA0000000;
	//u8 v = *invalid_addr;
}
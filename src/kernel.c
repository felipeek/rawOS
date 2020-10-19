#include "screen.h"
#include "interrupt.h"
#include "common.h"
#include "timer.h"
#include "keyboard.h"
#include "paging.h"
#include "alloc/kalloc.h"
#include "alloc/kalloc_test.h"
#include "fs/vfs.h"
#include "fs/initrd.h"
#include "util/printf.h"

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
	screen_print(logo);
	screen_print("\n\nWelcome to rawOS!\n");
}

void read_folder(Vfs_Node* folder_node) {
	Vfs_Dirent dirent;
	s32 index = 0;
	while (!vfs_readdir(folder_node, index++, &dirent)) {
		screen_print("Found file: ");
		screen_print(dirent.name);
		screen_print("\n");

		Vfs_Node* node = vfs_lookup(folder_node, dirent.name);

		screen_print("Content: ");

		const u32 buffer_size = 64;
		u8 buffer[buffer_size];
		s32 offset = 0;
		s32 read;

		while(read = vfs_read(node, offset, buffer_size - 1, buffer)) {
			offset += read;
			buffer[read] = 0;
			screen_print(buffer);
		}

		screen_print("\n");
	}
}

void main() {
	interrupt_init();
	timer_init();
	keyboard_init();
	screen_init();
	screen_clear();
	print_logo();
	paging_init();
	screen_print("Works like a charm :)\n");

	kalloc_init(1);
	catstring s = {0};
	int hello = catsprint(&s, "hello world %d %s\n\0", -45, "hello");
	//screen_print(s.data);
	//u8 buffer[32] = {0};
    //u32_to_str(45, (char*)buffer);
	screen_print(s.data);

	Vfs_Node* fs = initrd_init();
	read_folder(fs);

	//u8* a = kalloc_heap_alloc(&heap, 4 * 4096 + 1);
	//*(a + 4 * 4096) = 0xAB;
	//screen_print("Got ptr: ");
	//screen_print_ptr(a);
	//screen_print("\n");
	//screen_print("And value: ");
	//screen_print_byte(*(a + 4 * 4096));
	//screen_print("\n\n");

	//kalloc_heap_print(&heap);

	// Force page fault
	u8* invalid_addr = (u8*)0xA0000000;
	u8 v = *invalid_addr;
}
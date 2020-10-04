// reminder: once the kernel hits 512 bytes in size we will need to load more sectors.
// @TODO: fix this in QEMU
#include "screen.h"
#include "interrupt.h"
#include "common.h"
#include "timer.h"
#include "keyboard.h"
#include "paging.h"
#include "alloc/kalloc.h"
#include "alloc/kalloc_test.h"

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

// Keep this dummy value to force image to be bigger.
// Data is being stored at address 0x3000 and text at 0x1000, giving a 0x2000 delta between them.
u8 dummy = 0xAB;

void main() {
	interrupt_init();
	timer_init();
	keyboard_init();
	screen_init();
	screen_clear();
	print_logo();
	paging_init();
	screen_print("Works like a charm :)\n");

	Kalloc_Heap heap;
	kalloc_heap_create(&heap, 0x00500000, 1);
	kalloc_heap_print(&heap);

	kalloc_test(&heap);

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
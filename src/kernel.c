// reminder: once the kernel hits 512 bytes in size we will need to load more sectors.
// @TODO: fix this in QEMU
#include "screen.h"
#include "interrupt.h"
#include "common.h"
#include "timer.h"
#include "keyboard.h"

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
}
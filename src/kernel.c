// reminder: once the kernel hits 512 bytes in size we will need to load more sectors.
// @TODO: fix this in QEMU
#include "screen.h"
#include "interrupt.h"

void print_logo(Screen* s) {
	char logo[] =
"       -----------     ------    ---      ---   --------   ------------  \n"
"       ***********    ********   ***  **  ***  **********  ************  \n"
"       ----    ---   ----------  ---  --  --- ----    ---- ----          \n"
"       *********    ****    **** ***  **  *** ***      *** ************  \n"
"       ---------    ------------ ---  --  --- ---      --- ------------  \n"
"       ****  ****   ************ ************ ****    ****        *****  \n"
"       ----   ----  ----    ----  ----------   ----------  ------------  \n"
"       ****    **** ****    ****   ********     ********   ************  \n"
;
	screen_print(s, logo);
	screen_print(s, "\n\nWelcome to rawOS!\n");
}

// Keep this dummy value to force image to be bigger.
// Data is being stored at address 0x3000 and text at 0x1000, giving a 0x2000 delta between them.
unsigned char dummy = 0xAB;

// For now, let the screen be global.
Screen s;

void main() {
	screen_init(&s);
	screen_clear(&s);
	print_logo(&s);
    screen_print_ptr(&s, &s);
	//screen_print_byte_in_hex(&s, dummy);

	idt_init();
}
// reminder: once the kernel hits 512 bytes in size we will need to load more sectors.
// @TODO: fix this in QEMU
#include "screen.h"

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
	screen_print(s, "\n\nWelcome to rawOS!");
}

void main() {
	Screen s;
	screen_init(&s);
	screen_clear(&s);
	print_logo(&s);
}

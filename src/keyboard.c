#include "keyboard.h"
#include "interrupt.h"
#include "asm/io.h"
#include "keyboard_scancode_set_1.h"
#include "util/printf.h"

#define KEYBOARD_DATA_PORT 0x60                 // Read/Write port
#define KEYBOARD_STATUS_REGISTER_PORT 0x64      // Read port
#define KEYBOARD_COMMAND_REGISTER_PORT 0x64     // Write port

void keyboard_interrupt_handler(const Interrupt_Handler_Args* args) {
	// For now we interact with the keyboard device considering it is a PS/2 Keyboard, managed by the PS/2 Controller.
	// Nowadays they usually aren't PS/2, but for compatibility purposes, the mainboard supports USB Legacy Mode, meaning
	// that it emulates a PS/2 keyboard, even if it is a USB keyboard.
	// For the future, once we have a USB driver, we can disable the legacy mode and use the USB controller to interact with the keyboard.

	// When we receive an IRQ 1, we don't need to check the output buffer status in the status register
	// However, just for learning purposes, I'm confirming it is 1.
	u8 status = io_byte_in(KEYBOARD_STATUS_REGISTER_PORT);
	if (!(status & 0x01)) {
		printf("Error: Output buffer status is 0 after receiving the interrupt!\n");
	}

	// Get the scancode from the Keyboard device.
	u8 scan_code = io_byte_in(KEYBOARD_DATA_PORT);

	status = io_byte_in(KEYBOARD_STATUS_REGISTER_PORT);

	switch (scan_code) {
		case KEYBOARD_A_PRESS: printf("A"); break;
		case KEYBOARD_B_PRESS: printf("B"); break;
		case KEYBOARD_C_PRESS: printf("C"); break;
		case KEYBOARD_D_PRESS: printf("D"); break;
		case KEYBOARD_E_PRESS: printf("E"); break;
		case KEYBOARD_F_PRESS: printf("F"); break;
		case KEYBOARD_G_PRESS: printf("G"); break;
		case KEYBOARD_H_PRESS: printf("H"); break;
		case KEYBOARD_I_PRESS: printf("I"); break;
		case KEYBOARD_J_PRESS: printf("J"); break;
		case KEYBOARD_K_PRESS: printf("K"); break;
		case KEYBOARD_L_PRESS: printf("L"); break;
		case KEYBOARD_M_PRESS: printf("M"); break;
		case KEYBOARD_N_PRESS: printf("N"); break;
		case KEYBOARD_O_PRESS: printf("O"); break;
		case KEYBOARD_P_PRESS: printf("P"); break;
		case KEYBOARD_Q_PRESS: printf("Q"); break;
		case KEYBOARD_R_PRESS: printf("R"); break;
		case KEYBOARD_S_PRESS: printf("S"); break;
		case KEYBOARD_T_PRESS: printf("T"); break;
		case KEYBOARD_U_PRESS: printf("U"); break;
		case KEYBOARD_V_PRESS: printf("V"); break;
		case KEYBOARD_X_PRESS: printf("X"); break;
		case KEYBOARD_W_PRESS: printf("W"); break;
		case KEYBOARD_Y_PRESS: printf("Y"); break;
		case KEYBOARD_Z_PRESS: printf("Z"); break;
		case KEYBOARD_ENTER_PRESS: printf("\n"); break;
	}
}

void keyboard_init() {
	interrupt_register_handler(keyboard_interrupt_handler, IRQ1);
}
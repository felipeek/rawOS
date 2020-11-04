#include "keyboard.h"
#include "interrupt.h"
#include "asm/io.h"
#include "keyboard_scancode_set_1.h"
#include "util/printf.h"
#include "util/util.h"

#define KEYBOARD_DATA_PORT 0x60                 // Read/Write port
#define KEYBOARD_STATUS_REGISTER_PORT 0x64      // Read port
#define KEYBOARD_COMMAND_REGISTER_PORT 0x64     // Write port

#define KEYBOARD_EVENT_BUFFERS_MAX 64

static u32 keyboard_event_buffers_num = 0;
static Keyboard_Event_Receiver_Buffer* keyboard_event_buffers[KEYBOARD_EVENT_BUFFERS_MAX];

void keyboard_register_event_buffer(Keyboard_Event_Receiver_Buffer* kerb) {
	kerb->event_received = 0;
	kerb->buffer_filled = 0;
	keyboard_event_buffers[keyboard_event_buffers_num++] = kerb;
}

void keyboard_interrupt_handler(Interrupt_Handler_Args* args) {
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

	u8 out;
	switch (scan_code) {
		case KEYBOARD_A_PRESS: out = 'A'; break;
		case KEYBOARD_B_PRESS: out = 'B'; break;
		case KEYBOARD_C_PRESS: out = 'C'; break;
		case KEYBOARD_D_PRESS: out = 'D'; break;
		case KEYBOARD_E_PRESS: out = 'E'; break;
		case KEYBOARD_F_PRESS: out = 'F'; break;
		case KEYBOARD_G_PRESS: out = 'G'; break;
		case KEYBOARD_H_PRESS: out = 'H'; break;
		case KEYBOARD_I_PRESS: out = 'I'; break;
		case KEYBOARD_J_PRESS: out = 'J'; break;
		case KEYBOARD_K_PRESS: out = 'K'; break;
		case KEYBOARD_L_PRESS: out = 'L'; break;
		case KEYBOARD_M_PRESS: out = 'M'; break;
		case KEYBOARD_N_PRESS: out = 'N'; break;
		case KEYBOARD_O_PRESS: out = 'O'; break;
		case KEYBOARD_P_PRESS: out = 'P'; break;
		case KEYBOARD_Q_PRESS: out = 'Q'; break;
		case KEYBOARD_R_PRESS: out = 'R'; break;
		case KEYBOARD_S_PRESS: out = 'S'; break;
		case KEYBOARD_T_PRESS: out = 'T'; break;
		case KEYBOARD_U_PRESS: out = 'U'; break;
		case KEYBOARD_V_PRESS: out = 'V'; break;
		case KEYBOARD_X_PRESS: out = 'X'; break;
		case KEYBOARD_W_PRESS: out = 'W'; break;
		case KEYBOARD_Y_PRESS: out = 'Y'; break;
		case KEYBOARD_Z_PRESS: out = 'Z'; break;
		case KEYBOARD_ENTER_PRESS: out = '\n'; break;
		default: return;
	}

	for (u32 i = 0; i < keyboard_event_buffers_num; ++i) {
		Keyboard_Event_Receiver_Buffer* kerb = keyboard_event_buffers[i];
		memcpy(kerb->buffer, &out, sizeof(u8));
		kerb->event_received = 1;
		kerb->buffer_filled = sizeof(u8);
	}

	keyboard_event_buffers_num = 0;
}

void keyboard_init() {
	interrupt_register_handler(keyboard_interrupt_handler, IRQ1);
}
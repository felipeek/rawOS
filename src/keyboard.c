#include "keyboard.h"
#include "interrupt.h"
#include "asm/io.h"
#include "keyboard_scancode_set_1.h"
#include "util/printf.h"
#include "util/util.h"

#define KEYBOARD_DATA_PORT 0x60                 // Read/Write port
#define KEYBOARD_STATUS_REGISTER_PORT 0x64      // Read port
#define KEYBOARD_COMMAND_REGISTER_PORT 0x64     // Write port

#define KEY_LOOKUP_TABLE_SIZE 256
#define KEYBOARD_EVENT_BUFFERS_MAX 64

#define KEY_CHARACTER 0x1
#define KEY_CONTROL 0x2
#define KEY_UNKNOWN 0x4
#define KEY_PRESS 0x8
#define KEY_RELEASE 0x10

typedef struct {
	u8 key_code;
	u32 key_flags;
} Key_Information;

typedef struct {
	Key_Information key_lookup_table[KEY_LOOKUP_TABLE_SIZE];
	u32 keyboard_event_buffers_num;
	Keyboard_Event_Receiver_Buffer* keyboard_event_buffers[KEYBOARD_EVENT_BUFFERS_MAX];
	s32 shift_enabled;
} Keyboard_State;

static Keyboard_State keyboard_state;

void keyboard_register_event_buffer(Keyboard_Event_Receiver_Buffer* kerb) {
	kerb->event_received = 0;
	kerb->buffer_filled = 0;
	keyboard_state.keyboard_event_buffers[keyboard_state.keyboard_event_buffers_num++] = kerb;
}

static u8 get_key_character(u8 key_code) {
	if (key_code >= 'A' && key_code <= 'Z') {
		if (!keyboard_state.shift_enabled) {
			key_code += 32;
		}
	}
	return key_code;
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

	Key_Information key_information = keyboard_state.key_lookup_table[scan_code];
	if (key_information.key_flags & KEY_CHARACTER) {
		if (key_information.key_flags & KEY_PRESS) {
			u8 out = get_key_character(key_information.key_code);

			for (u32 i = 0; i < keyboard_state.keyboard_event_buffers_num; ++i) {
				Keyboard_Event_Receiver_Buffer* kerb = keyboard_state.keyboard_event_buffers[i];
				memcpy(kerb->buffer, &out, sizeof(u8));
				kerb->event_received = 1;
				kerb->buffer_filled = sizeof(u8);
			}

			keyboard_state.keyboard_event_buffers_num = 0;
		}
	} else if (key_information.key_flags & KEY_CONTROL) {
		if (key_information.key_flags & KEY_PRESS) {
			if (key_information.key_code == KEY_CODE_SHIFT) {
				keyboard_state.shift_enabled = 1;
			}
		} else if (key_information.key_flags & KEY_RELEASE) {
			if (key_information.key_code == KEY_CODE_SHIFT) {
				keyboard_state.shift_enabled = 0;
			}
		}
	}
}

void keyboard_init() {
	memset(&keyboard_state, 0, sizeof(Keyboard_State));

	for (u32 i = 0; i < KEY_LOOKUP_TABLE_SIZE; ++i) {
		keyboard_state.key_lookup_table[i] = (Key_Information){ .key_code = 0, .key_flags = KEY_UNKNOWN };
	}

	keyboard_state.key_lookup_table[KEYBOARD_A_PRESS] = (Key_Information){ .key_code = 'A', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_B_PRESS] = (Key_Information){ .key_code = 'B', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_C_PRESS] = (Key_Information){ .key_code = 'C', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_D_PRESS] = (Key_Information){ .key_code = 'D', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_E_PRESS] = (Key_Information){ .key_code = 'E', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_F_PRESS] = (Key_Information){ .key_code = 'F', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_G_PRESS] = (Key_Information){ .key_code = 'G', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_H_PRESS] = (Key_Information){ .key_code = 'H', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_I_PRESS] = (Key_Information){ .key_code = 'I', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_J_PRESS] = (Key_Information){ .key_code = 'J', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_K_PRESS] = (Key_Information){ .key_code = 'K', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_L_PRESS] = (Key_Information){ .key_code = 'L', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_M_PRESS] = (Key_Information){ .key_code = 'M', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_N_PRESS] = (Key_Information){ .key_code = 'N', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_O_PRESS] = (Key_Information){ .key_code = 'O', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_P_PRESS] = (Key_Information){ .key_code = 'P', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_Q_PRESS] = (Key_Information){ .key_code = 'Q', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_R_PRESS] = (Key_Information){ .key_code = 'R', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_S_PRESS] = (Key_Information){ .key_code = 'S', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_T_PRESS] = (Key_Information){ .key_code = 'T', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_U_PRESS] = (Key_Information){ .key_code = 'U', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_V_PRESS] = (Key_Information){ .key_code = 'V', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_X_PRESS] = (Key_Information){ .key_code = 'X', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_W_PRESS] = (Key_Information){ .key_code = 'W', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_Y_PRESS] = (Key_Information){ .key_code = 'Y', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_Z_PRESS] = (Key_Information){ .key_code = 'Z', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_A_RELEASE] = (Key_Information){ .key_code = 'A', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_B_RELEASE] = (Key_Information){ .key_code = 'B', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_C_RELEASE] = (Key_Information){ .key_code = 'C', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_D_RELEASE] = (Key_Information){ .key_code = 'D', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_E_RELEASE] = (Key_Information){ .key_code = 'E', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_F_RELEASE] = (Key_Information){ .key_code = 'F', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_G_RELEASE] = (Key_Information){ .key_code = 'G', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_H_RELEASE] = (Key_Information){ .key_code = 'H', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_I_RELEASE] = (Key_Information){ .key_code = 'I', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_J_RELEASE] = (Key_Information){ .key_code = 'J', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_K_RELEASE] = (Key_Information){ .key_code = 'K', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_L_RELEASE] = (Key_Information){ .key_code = 'L', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_M_RELEASE] = (Key_Information){ .key_code = 'M', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_N_RELEASE] = (Key_Information){ .key_code = 'N', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_O_RELEASE] = (Key_Information){ .key_code = 'O', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_P_RELEASE] = (Key_Information){ .key_code = 'P', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_Q_RELEASE] = (Key_Information){ .key_code = 'Q', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_R_RELEASE] = (Key_Information){ .key_code = 'R', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_S_RELEASE] = (Key_Information){ .key_code = 'S', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_T_RELEASE] = (Key_Information){ .key_code = 'T', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_U_RELEASE] = (Key_Information){ .key_code = 'U', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_V_RELEASE] = (Key_Information){ .key_code = 'V', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_X_RELEASE] = (Key_Information){ .key_code = 'X', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_W_RELEASE] = (Key_Information){ .key_code = 'W', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_Y_RELEASE] = (Key_Information){ .key_code = 'Y', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_Z_RELEASE] = (Key_Information){ .key_code = 'Z', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_ENTER_PRESS] = (Key_Information){ .key_code = '\n', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_ENTER_RELEASE] = (Key_Information){ .key_code = '\n', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_SPACE_PRESS] = (Key_Information){ .key_code = ' ', .key_flags = KEY_CHARACTER | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_SPACE_RELEASE] = (Key_Information){ .key_code = ' ', .key_flags = KEY_CHARACTER | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_LEFT_SHIFT_PRESS] = (Key_Information){ .key_code = KEY_CODE_SHIFT, .key_flags = KEY_CONTROL | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_LEFT_SHIFT_RELEASE] = (Key_Information){ .key_code = KEY_CODE_SHIFT, .key_flags = KEY_CONTROL | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_RIGHT_SHIFT_PRESS] = (Key_Information){ .key_code = KEY_CODE_SHIFT, .key_flags = KEY_CONTROL | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_RIGHT_SHIFT_RELEASE] = (Key_Information){ .key_code = KEY_CODE_SHIFT, .key_flags = KEY_CONTROL | KEY_RELEASE };
	keyboard_state.key_lookup_table[KEYBOARD_LEFT_CONTROL_PRESS] = (Key_Information){ .key_code = KEY_CODE_CTRL, .key_flags = KEY_CONTROL | KEY_PRESS };
	keyboard_state.key_lookup_table[KEYBOARD_LEFT_CONTROL_RELEASE] = (Key_Information){ .key_code = KEY_CODE_CTRL, .key_flags = KEY_CONTROL | KEY_RELEASE };
	interrupt_register_handler(keyboard_interrupt_handler, IRQ1);
}
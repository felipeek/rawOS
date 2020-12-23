#ifndef RAW_OS_KEYBOARD_H
#define RAW_OS_KEYBOARD_H
#include "common.h"

#define KEY_CODE_SHIFT 1
#define KEY_CODE_CTRL 1

typedef struct {
	u8* buffer;
	u32 buffer_capacity;
	u32 buffer_filled;
	u32 event_received;
	u32 pid;
} Keyboard_Event_Receiver_Buffer;

void keyboard_init();
void keyboard_register_event_buffer(Keyboard_Event_Receiver_Buffer* kerb);
#endif
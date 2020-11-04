#ifndef RAW_OS_KEYBOARD_H
#define RAW_OS_KEYBOARD_H
#include "common.h"

typedef struct {
	u8* buffer;
	u32 buffer_capacity;
	u32 buffer_filled;
	u32 event_received;
} Keyboard_Event_Receiver_Buffer;

void keyboard_init();
void keyboard_register_event_buffer(Keyboard_Event_Receiver_Buffer* kerb);
#endif
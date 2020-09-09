#include "util.h"
#include "screen.h"

void util_memcpy(void* dst, void* src, u32 size) {
	for (u32 i = 0; i < size; ++i) {
		*((s8*)dst + i) = *((s8*)src + i);
	}
}

void util_memset(void* ptr, u8 value, u32 num) {
	for (u32 i = 0; i < num; ++i) {
		*((s8*)ptr + i) = value;
	}
}

void util_panic(s8* message) {
	screen_print("************ PANIC ************\n");
	screen_print(message);
	while (1) {}
}
#include "util.h"
#include "../screen.h"

void util_strcpy(s8* dst, const s8* src) {
	s32 i = 0;
	while (src[i]) {
		dst[i] = src[i];
		++i;
	}
	dst[i] = 0;
}

// For now implementation differs a bit from the CRT.
// Simply returns 1 if different. 0 if equal.
s32 util_strcmp(const s8* s1, const s8* s2) {
	s32 i = 0;
	while (s1[i] && s2[i]) {
		if (s1[i] != s2[i]) {
			return 1;
		}
		++i;
	}

	return !(s1[i] == 0 && s2[i] == 0);
}

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

void util_assert(const s8* message, int condition) {
	if (!condition) {
		screen_print("Assert failed: ");
		screen_print(message);
		screen_print("\n");
		util_panic("Assert failed");
	}
}

void util_panic(const s8* message) {
	screen_print("************ PANIC ************\n");
	screen_print(message);
	while (1) {}
}
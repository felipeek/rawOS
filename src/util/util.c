#include "util.h"
#include "printf.h"

void strcpy(s8* dst, const s8* src) {
	s32 i = 0;
	while (src[i]) {
		dst[i] = src[i];
		++i;
	}
	dst[i] = 0;
}

u32 strlen(const s8* str) {
	s32 len = 0;
	while (str[len]) ++len;
	return len;
}

// For now implementation differs a bit from the CRT.
// Simply returns 1 if different. 0 if equal.
s32 strcmp(const s8* s1, const s8* s2) {
	s32 i = 0;
	while (s1[i] && s2[i]) {
		if (s1[i] != s2[i]) {
			return 1;
		}
		++i;
	}

	return !(s1[i] == 0 && s2[i] == 0);
}

void memcpy(void* dst, const void* src, u32 size) {
	for (u32 i = 0; i < size; ++i) {
		*((s8*)dst + i) = *((s8*)src + i);
	}
}

void memset(void* ptr, u8 value, u32 num) {
	for (u32 i = 0; i < num; ++i) {
		*((s8*)ptr + i) = value;
	}
}

void assert(s32 condition, const s8* message, ...) {
	if (!condition) {
		printf("Assert failed: ");
		rawos_va_list args;
		rawos_va_start(args, message);
		vprintf(message, args);
		rawos_va_end(args);
		printf("\n");
		panic("Assert failed");
	}
}

void panic(const s8* message) {
	printf("************ PANIC ************\n");
	printf(message);
	while (1) {}
}
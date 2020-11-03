#include "printf.h"
#include "../screen.h"
#include "util.h"

static s32 u32_to_str_base16(u32 value, s32 bitsize, s32 leading_zeros, s8 buffer[16])
{
	s32 i = 0;
	u32 mask = 0xf0000000 >> (32 - bitsize);

	for (; i < (bitsize / 4); i += 1) {
		u32 f = (value & mask) >> (bitsize - 4);
		if (f > 9) {
			buffer[i] = (u8)f + 0x57; //0x37;
		} else {
			buffer[i] = (u8)f + 0x30;
		}
		value = value << 4;
	}

	return i;
}

static s32 u32_to_str(u32 val, s8 buffer[16]) {
	s8 b[16] = {0};
	s32 sum = 0;

	if (val == 0) {
		buffer[0] = '0';
		return 1;
	}

	s8 *auxbuffer = &b[15];
	s8 *start = auxbuffer + sum;

	while (val != 0) {
		u32 rem = val % 10;
		val /= 10;
		*auxbuffer = '0' + (s8)rem;
		auxbuffer -= 1;
	}

	s32 size = start - auxbuffer;

	memcpy(&buffer[sum], auxbuffer + 1, (u32)size);
	return size;
}

static s32 s32_to_str(s32 val, s8 buffer[16]) {
	s8 b[16] = {0};
	s32 sum = 0;

	if (val == 0) {
		buffer[0] = '0';
		return 1;
	}

	if (val < 0) {
		val = -val;
		buffer[0] = '-';
		sum = 1;
	}

	s8 *auxbuffer = &b[15];
	s8 *start = auxbuffer + sum;

	while (val != 0) {
		s32 rem = val % 10;
		val /= 10;
		*auxbuffer = '0' + (s8)rem;
		--auxbuffer;
	}

	s32 size = start - auxbuffer;
	memcpy(&buffer[sum], auxbuffer + 1, (u32)size);
	return size;
}

static void print_string(const s8 *str) {
	for (const s8 *at = str; *at != 0; ++at) {
		screen_print_char(*at);
	}
}

static void print_decimal_unsigned(u32 value) {
	s8 mem[16] = {0};
	u32_to_str(value, mem);
	screen_print(mem);
}

static void print_decimal_signed(s32 value) {
	s8 mem[16] = {0};
	s32_to_str(value, mem);
	screen_print(mem);
}

static void print_hexadecimal(u32 value) {
	s8 mem[16] = {0};
	u32_to_str_base16(value, 32, 1, mem);
	screen_print(mem);
}

void vprintf(const s8 *str, rawos_va_list args) {
	for (const s8 *at = str; *at != 0; ++at) {
		if (*at == '%') {
			++at; // skip
			if (!(*at)) {
				break;
			}

			switch (*at) {
				case '%': {
					// just print character
					screen_print_char(*at);
				} break;
				case 's': {
					// print string
					print_string(rawos_va_arg(args, const s8 *));
				} break;
				case 'u': {
					print_decimal_unsigned(rawos_va_arg(args, u32));
				} break;
				case 'd': {
					print_decimal_signed(rawos_va_arg(args, s32));
				} break;
				case 'x': {
					print_hexadecimal(rawos_va_arg(args, u32));
				} break;
			}
		} else {
			screen_print_char(*at);
		}
	}

	return;
}

void printf(const s8* str, ...) {
	rawos_va_list args;
	rawos_va_start(args, str);

	vprintf(str, args);

	rawos_va_end(args);
}
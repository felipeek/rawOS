#include "printf.h"
#include "../screen.h"
#include "../alloc/kalloc.h"
#include "util.h"

typedef struct {
	s32 length;
	s32 capacity;
	s8* data;
} Printf_Buffer;

static void buffer_grow(Printf_Buffer *s) {
	if (s->capacity < 32) {
		s->data = kalloc_realloc(s->data, s->capacity, s->capacity + 32);
		s->capacity = s->capacity + 32;
	} else {
		s->data = kalloc_realloc(s->data, s->capacity, s->capacity * 2);
		s->capacity = s->capacity * 2;
	}
}

static void buffer_grow_by(Printf_Buffer *s, s32 amount) {
	u32 a = (s->capacity + amount) * 2;
	s->data = kalloc_realloc(s->data, s->capacity, a);
	s->capacity = a;
}

static s32 u32_to_str_base16(u32 value, s32 bitsize, s32 leading_zeros, s32 capitalized, u8 buffer[16])
{
	s32 i = 0;
	u32 mask = 0xf0000000 >> (32 - bitsize);
	u8 cap = 0x57;
	if (capitalized) {
		cap = 0x37; // hex letters capitalized
	}

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

static s32 u32_to_str(u32 val, u8* buffer) {
	u8 b[16] = {0};
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
		*auxbuffer = '0' + (u8)rem;
		auxbuffer -= 1;
	}

	s32 size = start - auxbuffer;

	util_memcpy(&buffer[sum], auxbuffer + 1, (u32)size);
	return size;
}

static s32 s32_to_str(s32 val, u8* buffer) {
	u8 b[16] = {0};
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
		*auxbuffer = '0' + (u8)rem;
		--auxbuffer;
	}

	s32 size = start - auxbuffer;
	util_memcpy(&buffer[sum], auxbuffer + 1, (u32)size);
	return size;
}

// TODO(psv): very big numbers are incorrect
static s32 r32_to_str(r32 v, u8 buffer[32]) {
	u32 l = 0;
	if (v < 0.0) {
		buffer[l] = '-';
		l += 1;
		v = -v;
	}

	l += s32_to_str((s32)v, ((u8 *)buffer + l));
	s32 precision = 6;

	r32 fractional_part = v - ((r32)((s32)v));

	buffer[l] = '.';
	l += 1;

	if (fractional_part == 0.0) {
		buffer[l] = '0';
		l += 1;
		return l;
	}

	while (precision > 0 && fractional_part > 0.0) {
		fractional_part *= 10.0;
		buffer[l] = (u8)fractional_part + '0';
		fractional_part = fractional_part - ((r32)((u8)fractional_part));
		l += 1;
		precision -= 1;
	}

	return l;
}

// TODO(psv): very big numbers are incorrect
static s32 r64_to_str(r64 v, u8 buffer[64]) {
	s32 l = 0;
	if (v < 0.0) {
		buffer[l] = '-';
		l += 1;
		v = -v;
	}

	l += s32_to_str((s32)v, ((u8 *)buffer + l));
	s32 precision = 6;

	r64 fractional_part = v - ((r64)((s32)v));

	buffer[l] = '.';
	l += 1;

	if (fractional_part == 0.0) {
		buffer[l] = '0';
		l += 1;
		return l;
	}

	while (precision > 0 && fractional_part > 0.0) {
		fractional_part *= 10.0;
		buffer[l] = (u8)fractional_part + '0';
		fractional_part = fractional_part - ((r64)((s32)fractional_part));
		l += 1;
		precision -= 1;
	}
	return l;
}

static s32 sprint_string_length(Printf_Buffer *buffer, const s8 *str, s32 length) {
	if (buffer->capacity == 0) {
		buffer->data = kalloc_alloc(MAX(32, length));
		buffer->capacity = MAX(32, length);
	}

	s32 n = 0;

	if ((buffer->length + length) >= buffer->capacity) {
		buffer_grow_by(buffer, length);
	}
	util_memcpy(buffer->data + buffer->length, str, length);
	buffer->length += length;

	return length;
}

static s32 sprint_string_length_escaped(Printf_Buffer *buffer, const s8 *str, s32 length) {
	if (buffer->capacity == 0) {
		buffer->data = kalloc_alloc(MAX(32, length));
		buffer->capacity = MAX(32, length);
	}

	s32 n = 0;

	if ((buffer->length + length) >= buffer->capacity) {
		buffer_grow_by(buffer, length * 2);
	}

	s8 *at = buffer->data + buffer->length;
	for (s32 i = 0; i < length; ++i) {
		s8 c = *(str + i);
		if (c == '\n') {
			*at = '\\';
			at++;
			*at = 'n';
			buffer->length++;
		} else {
			*at = c;
		}
		at++;
	}
	buffer->length += length;

	return length;
}

static s32 sprint_string(Printf_Buffer *buffer, const s8 *str) {
	if (buffer->capacity == 0) {
		buffer->data = kalloc_alloc(32);
		buffer->capacity = 32;
	}

	s32 n = 0;
	for (const s8 *at = str;; ++at, ++n) {
		// push to stream
		if (buffer->length >= buffer->capacity) {
			buffer_grow(buffer);
		}

		buffer->data[buffer->length] = *at;
		if (!(*at)) {
			break;
		}
		buffer->length++;
	}

	return n;
}

static s32 sprint_decimal_unsigned(Printf_Buffer *buffer, u32 value) {
	if (buffer->capacity == 0) {
		buffer->data = kalloc_alloc(32);
		buffer->capacity = 32;
	}

	u8 mem[32] = {0};
	s32 length = u32_to_str(value, mem);
	return sprint_string_length(buffer, (const s8 *)mem, length);
}

static s32 sprint_decimal_signed(Printf_Buffer *buffer, s32 value) {
	if (buffer->capacity == 0) {
		buffer->data = kalloc_alloc(32);
		buffer->capacity = 32;
	}

	u8 mem[32] = {0};
	s32 length = s32_to_str(value, mem);
	return sprint_string_length(buffer, mem, length);
}

static s32 sprint_hexadecimal(Printf_Buffer *buffer, u32 value) {
	if (buffer->capacity == 0) {
		buffer->data = kalloc_alloc(32);
		buffer->capacity = 32;
	}

	s8 mem[16] = {0};
	s32 length = u32_to_str_base16(value, 32, 1, 0, mem);
	return sprint_string_length(buffer, mem, length);
}

static s32 sprint_double(Printf_Buffer *buffer, double value) {
	if (buffer->capacity == 0) {
		buffer->data = kalloc_alloc(32);
		buffer->capacity = 32;
	}

	u8 mem[64] = {0};
	s32 length = r64_to_str(value, mem);
	return sprint_string_length(buffer, mem, length);
}

static s32 sprint_list(Printf_Buffer *buffer, const s8 *str, rawos_va_list args) {
	if (buffer->capacity == 0) {
		buffer->data = kalloc_alloc(32);
		buffer->capacity = 32;
	}

	s32 len = 0;
	for (const s8 *at = str;;) {
		// push to stream
		if (buffer->length >= buffer->capacity) {
			buffer_grow(buffer);
		}

		s32 n = 0;

		if (*at == '%') {
			at++; // skip
			switch (*at) {
				case '%': {
					// just push character
					buffer->data[buffer->length] = *at;
					if (!(*at)) {
						break;
					}
					buffer->length++;
					len++;
					at++;
				} break;
				case 's': {
					at++;
					if (*at == '+') {
						at++;
						s32 length = rawos_va_arg(args, s32);
						// push string
						n = sprint_string_length(buffer, rawos_va_arg(args, const s8 *), length);
					} else if (*at == '*') {
						at++;
						s32 length = rawos_va_arg(args, s32);
						// push string
						n = sprint_string_length_escaped(buffer, rawos_va_arg(args, const s8 *), length);
					} else {
						// push string
						n = sprint_string(buffer, rawos_va_arg(args, const s8 *));
					}
				} break;
				case 'u': {
					at++;
					n = sprint_decimal_unsigned(buffer, rawos_va_arg(args, u32));
				} break;
				case 'd': {
					at++;
					n = sprint_decimal_signed(buffer, rawos_va_arg(args, s32));
				} break;
				case 'x': {
					at++;
					n = sprint_hexadecimal(buffer, rawos_va_arg(args, u32));
				} break;
				case 'f': {
					at++;
					n = sprint_double(buffer, rawos_va_arg(args, double));
				} break;
			}
			len += n;
		} else if (*at == '\\' && at[1] == '0') {
			buffer->data[buffer->length] = 0;
			if (!(*at)) {
				break;
			}
			buffer->length++;
			len++;
			at += 2;
		} else {
			buffer->data[buffer->length] = *at;
			if (!(*at)) {
				break;
			}
			buffer->length++;
			len++;
			at++;
		}
	}

	return len;
}

static void printf_buffer_free(Printf_Buffer *s) {
	kalloc_free(s->data);
	s->capacity = 0;
	s->length = 0;
}

void printf(const s8* str, ...) {
	Printf_Buffer buffer = {0};

	rawos_va_list args;
	rawos_va_start(args, str);

	sprint_list(&buffer, str, args);

	rawos_va_end(args);

	screen_print(buffer.data);
	printf_buffer_free(&buffer);
}
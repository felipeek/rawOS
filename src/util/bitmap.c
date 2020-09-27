#include "bitmap.h"
#include "util.h"
#include "../screen.h"

void bitmap_print(const Bitmap* bitmap) {
	screen_print("Bitmap: ");
	for (u32 i = 0; i < bitmap->size; ++i) {
		screen_print_byte(bitmap->data[i]);
	}
	screen_print("\n");
}

void bitmap_init(Bitmap* bitmap, unsigned char* data, u32 size) {
	bitmap->data = data;
	bitmap->size = size;
}

void bitmap_set(Bitmap* bitmap, u32 index) {
	u32 bitmap_index = index / 8;
	u32 bitmap_bit = index % 8;
	util_assert("bitmap_set: bitmap_index < bitmap_size", bitmap_index < bitmap->size * 8);
	bitmap->data[bitmap_index] |= (1 << bitmap_bit);
}

void bitmap_clear(Bitmap* bitmap, u32 index) {
	u32 bitmap_index = index / 8;
	u32 bitmap_bit = index % 8;
	util_assert("bitmap_clear: bitmap_index < bitmap_size", bitmap_index < bitmap->size * 8);
	bitmap->data[bitmap_index] &= ~(1 << bitmap_bit);
}

u32 bitmap_get_first_clear(const Bitmap* bitmap) {
	for (u32 i = 0; i < bitmap->size; ++i) {
		for (u32 j = 0; j < 8; ++j) {
			if (!(bitmap->data[i] & (1 << j))) {
				return i * 8 + j;
			}
		}
	}

	util_panic("Bitmap full!");
}

u32 bitmap_get(const Bitmap* bitmap, u32 index) {
	u32 bitmap_index = index / 8;
	u32 bitmap_bit = index % 8;
	util_assert("bitmap_get: bitmap_index < bitmap_size", bitmap_index < bitmap->size * 8);
	return (bitmap->data[bitmap_index] & (1 << bitmap_bit)) != 0;
}
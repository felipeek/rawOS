#ifndef RAW_OS_UTIL_BITMAP_H
#include "../common.h"

typedef struct {
	unsigned char* data;
	u32 size;
} Bitmap;

// NOTE: the size is given in BYTES. So, in order to create a bitmap with 32 elements, size would be 4.
void bitmap_init(Bitmap* bitmap, unsigned char* data, u32 size);
void bitmap_set(Bitmap* bitmap, u32 index);
void bitmap_clear(Bitmap* bitmap, u32 index);
u32 bitmap_get_first_clear(const Bitmap* bitmap);
void bitmap_print(const Bitmap* bitmap);
#endif
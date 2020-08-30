#ifndef RAW_OS_SCREEN_H
#define RAW_OS_SCREEN_H
#include "common.h"
typedef struct {
    s32 cursor_pos;
} Screen;

void screen_init(Screen* screen);
void screen_clear(Screen* screen);
void screen_print(Screen* screen, const s8* str);
void screen_print_ptr(Screen* screen, void* ptr);
void screen_print_byte(Screen* screen, u8 byte);
void screen_print_u32(Screen* screen, u32 v);
#endif
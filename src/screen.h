#ifndef RAW_OS_SCREEN_H
#define RAW_OS_SCREEN_H
#include "common.h"

void screen_init();
void screen_clear();
void screen_print(const s8* str);
void screen_print_ptr(void* ptr);
void screen_print_byte(u8 byte);
void screen_print_u32(u32 v);
#endif
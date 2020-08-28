#ifndef RAW_OS_SCREEN_H
#define RAW_OS_SCREEN_H

typedef struct {
    int cursor_pos;
} Screen;

void screen_init(Screen* screen);
void screen_clear(Screen* screen);
void screen_print(Screen* screen, const char* str);
void screen_print_ptr(Screen* screen, void* ptr);
void screen_print_byte(Screen* screen, unsigned char byte);
#endif
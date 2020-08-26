#include "screen.h"
#include "io.h"

#define VIDEO_MEMORY_ADDRESS 0xB8000
#define WHITE_ON_BLACK_ATTRIBUTE 0x0F
#define VIDEO_COLS_NUM 80
#define VIDEO_ROWS_NUM 25
#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

// @TODO(fek): rewrite this temporary code
static void screen_print_hex(Screen* screen, unsigned char byte) {
    unsigned char nibble1 = byte >> 4;
    unsigned char nibble2 = byte & 0x0F;
    unsigned char aux[2];
    aux[1] = 0;
    screen_print(screen, "0x");
    if (nibble1 > 9) {
        aux[0] = nibble1 - 10 + 'A';
    } else {
        aux[0] = nibble1 + '0';
    }
    screen_print(screen, aux);
    if (nibble2 > 9) {
        aux[0] = nibble2 - 10 + 'A';
    } else {
        aux[0] = nibble2 + '0';
    }
    screen_print(screen, aux);
    screen_print(screen, "\n");
}

static void update_text_cursor(Screen* screen) {
    io_byte_out(REG_SCREEN_CTRL, 0x0E);
    io_byte_out(REG_SCREEN_DATA, screen->cursor_pos >> 8);
    io_byte_out(REG_SCREEN_CTRL, 0x0F);
    io_byte_out(REG_SCREEN_DATA, screen->cursor_pos);
}

static void print_at(char c, int y, int x) {
    char* video_memory = (char*)VIDEO_MEMORY_ADDRESS;
    video_memory[(y * VIDEO_COLS_NUM + x) * 2] = c;
    video_memory[(y * VIDEO_COLS_NUM + x) * 2 + 1] = WHITE_ON_BLACK_ATTRIBUTE;
}

void screen_init(Screen* screen) {
    screen->cursor_pos = 0;

    // Configure text cursor

    // 0x0A:
    // 7-6: Reserved
    // 5: Text Cursor Off (0 to enable text cursor, 1 otherwise)
    // 4-0: Text Cursor Start, specifies which horizontal line of pixels in a character box
    // is to be used to display the first horizontal line of the cursor in text mode.
    // The value specified by these 5 bits should be the number of the first horizontal line of pixels on which the cursor is to be shown
    io_byte_out(REG_SCREEN_CTRL, 0x0A);
    io_byte_out(REG_SCREEN_DATA, 0b00001111);

    // 0x0B:
    // 7: Reserved
    // 6-5: Text Cursor Skew: 00 no delay, 01 delay by 1 char clock, 10 delay by 2 char clocks, 11 delay by 3 char clocks
    // 4-0: Text Cursor End, specifies which horizontal line of pixels in a character box
    // is to be used to display the last horizontal line of the cursor in text mode.
    // The value specified by these 5 bits should be the number of the last horizontal line of pixels on which the cursor is to be shown
    io_byte_out(REG_SCREEN_CTRL, 0x0B);
    io_byte_out(REG_SCREEN_DATA, 0b00011111);
}

void screen_print(Screen* screen, const char* str) {
    char c;
    int i = 0;
    while ((c = str[i]) != '\0') {
        ++i;

        if (c == '\n') {
            screen->cursor_pos += VIDEO_COLS_NUM - (screen->cursor_pos % VIDEO_COLS_NUM);
            continue;
        }
        int y = screen->cursor_pos / VIDEO_COLS_NUM;
        int x = screen->cursor_pos % VIDEO_COLS_NUM;
        print_at(c, y, x);
        ++screen->cursor_pos;
    }

    update_text_cursor(screen);
}


void screen_clear(Screen* screen) {
    for (int i = 0; i < VIDEO_COLS_NUM * VIDEO_ROWS_NUM; ++i) {
        char* video_memory = (char*)VIDEO_MEMORY_ADDRESS;
        video_memory[2 * i] = 0;
        video_memory[2 * i + 1] = 0xF;
    }

    screen->cursor_pos = 0;
    update_text_cursor(screen);
}
#include "screen.h"
#include "io.h"
#include "util.h"

#define VIDEO_MEMORY_ADDRESS 0xB8000
#define WHITE_ON_BLACK_ATTRIBUTE 0x0F
#define VIDEO_COLS_NUM 80
#define VIDEO_ROWS_NUM 25
#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

// @TODO(fek): rewrite this temporary code
static void screen_print_nibble(Screen* screen, unsigned char nibble) {
    unsigned char data[2];
    data[0] = nibble + '0';
    if (nibble > 9) {
        data[0] += 7;
    }
    data[1] = 0;
    screen_print(screen, data);
}

void screen_print_byte(Screen* screen, unsigned char byte) {
    screen_print_nibble(screen, byte >> 4);
    screen_print_nibble(screen, byte & 0x0F);
}

void screen_print_ptr(Screen* screen, void* ptr) {
    unsigned int ptr_val = (unsigned int)ptr;
    screen_print_byte(screen, ptr_val >> 24);
    screen_print_byte(screen, ptr_val >> 16);
    screen_print_byte(screen, ptr_val >> 8);
    screen_print_byte(screen, ptr_val >> 0);
}

static void update_text_cursor(Screen* screen) {
    // Send 8 most-significant bytes of the cursor position
    io_byte_out(REG_SCREEN_CTRL, 0x0E);
    io_byte_out(REG_SCREEN_DATA, screen->cursor_pos >> 8);
    // Send 8 less-significant bytes of the cursor position
    io_byte_out(REG_SCREEN_CTRL, 0x0F);
    io_byte_out(REG_SCREEN_DATA, screen->cursor_pos);
}

// Prints a single character at the screen, in position <x, y>
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

// Shift all the content one line up, effectively losing the first line and creating space for a new line in the bottom
static void shift_one_line_up(Screen* screen) {
    char* video_memory = (char*)VIDEO_MEMORY_ADDRESS;
    util_memcpy(video_memory, video_memory + VIDEO_COLS_NUM * 2, (VIDEO_ROWS_NUM - 1) * VIDEO_COLS_NUM * 2);
    for (int i = 0; i < VIDEO_COLS_NUM; ++i) {
        video_memory[2 * ((VIDEO_ROWS_NUM - 1) * VIDEO_COLS_NUM + i)] = 0;
        video_memory[2 * ((VIDEO_ROWS_NUM - 1) * VIDEO_COLS_NUM + i) + 1] = 0xF;
    }
}

// Print a nul-terminated string to the screen, starting at the current cursor position
void screen_print(Screen* screen, const char* str) {
    char c;
    int i = 0;
    while ((c = str[i]) != '\0') {
        ++i;

        if (c == '\n') {
            // If the string has a \n, we artificially break the line
            screen->cursor_pos += VIDEO_COLS_NUM - (screen->cursor_pos % VIDEO_COLS_NUM);
        } else {
            // In the common case, we just print the character to the 
            int y = screen->cursor_pos / VIDEO_COLS_NUM;
            int x = screen->cursor_pos % VIDEO_COLS_NUM;
            print_at(c, y, x);
            ++screen->cursor_pos;
        }

        // If we reached the end of the screen, we shift everything one line up
        if (screen->cursor_pos >= VIDEO_COLS_NUM * VIDEO_ROWS_NUM) {
            screen->cursor_pos -= VIDEO_COLS_NUM;
            shift_one_line_up(screen);
        }
    }

    // Update the text cursor in the device
    update_text_cursor(screen);
}

// Clear the screen and reset the cursor position
void screen_clear(Screen* screen) {
    char* video_memory = (char*)VIDEO_MEMORY_ADDRESS;

    for (int i = 0; i < VIDEO_COLS_NUM * VIDEO_ROWS_NUM; ++i) {
        video_memory[2 * i] = 0;
        video_memory[2 * i + 1] = 0xF;
    }

    screen->cursor_pos = 0;
    update_text_cursor(screen);
}
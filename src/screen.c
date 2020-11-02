#include "screen.h"
#include "asm/io.h"
#include "util/util.h"

#define VIDEO_MEMORY_ADDRESS 0xB8000
#define WHITE_ON_BLACK_ATTRIBUTE 0x0F
#define VIDEO_COLS_NUM 80
#define VIDEO_ROWS_NUM 25
#define REG_SCREEN_CTRL 0x3D4
#define REG_SCREEN_DATA 0x3D5

typedef struct {
	s32 cursor_pos;
} Screen;

static Screen screen;

void screen_print_char(s8 c) {
	s8 buf[2];
	buf[0] = c;
	buf[1] = 0;
	screen_print(buf);
}

static void screen_print_nibble(u8 nibble) {
	u8 data[2];
	data[0] = nibble + '0';
	if (nibble > 9) {
		data[0] += 7;
	}
	data[1] = 0;
	screen_print(data);
}

void screen_print_byte(u8 byte) {
	screen_print_nibble(byte >> 4);
	screen_print_nibble(byte & 0x0F);
}

void screen_print_u32(u32 v) {
	screen_print_byte(v >> 24);
	screen_print_byte(v >> 16);
	screen_print_byte(v >> 8);
	screen_print_byte(v >> 0);
}

void screen_print_ptr(void* ptr) {
	screen_print_u32((u32)ptr);
}

static void update_text_cursor() {
	// Send 8 most-significant bytes of the cursor position
	io_byte_out(REG_SCREEN_CTRL, 0x0E);
	io_byte_out(REG_SCREEN_DATA, screen.cursor_pos >> 8);
	// Send 8 less-significant bytes of the cursor position
	io_byte_out(REG_SCREEN_CTRL, 0x0F);
	io_byte_out(REG_SCREEN_DATA, screen.cursor_pos);
}

// Prints a single character at the screen, in position <x, y>
static void print_at(s8 c, s32 y, s32 x) {
	s8* video_memory = (s8*)VIDEO_MEMORY_ADDRESS;
	video_memory[(y * VIDEO_COLS_NUM + x) * 2] = c;
	video_memory[(y * VIDEO_COLS_NUM + x) * 2 + 1] = WHITE_ON_BLACK_ATTRIBUTE;
}

void screen_pos_cursor(u32 x, u32 y) {
	assert(x < VIDEO_COLS_NUM, "error setting screen X cursor position: %u must be smaller than %u.", x, VIDEO_COLS_NUM);
	assert(y < VIDEO_ROWS_NUM, "error setting screen Y cursor position: %u must be smaller than %u.", y, VIDEO_ROWS_NUM);
	screen.cursor_pos = VIDEO_COLS_NUM * y + x;
}

void screen_init() {
	screen.cursor_pos = 0;

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
static void shift_one_line_up() {
	s8* video_memory = (s8*)VIDEO_MEMORY_ADDRESS;
	memcpy(video_memory, video_memory + VIDEO_COLS_NUM * 2, (VIDEO_ROWS_NUM - 1) * VIDEO_COLS_NUM * 2);
	for (s32 i = 0; i < VIDEO_COLS_NUM; ++i) {
		video_memory[2 * ((VIDEO_ROWS_NUM - 1) * VIDEO_COLS_NUM + i)] = 0;
		video_memory[2 * ((VIDEO_ROWS_NUM - 1) * VIDEO_COLS_NUM + i) + 1] = 0xF;
	}
}

// Print a nul-terminated string to the screen, starting at the current cursor position
void screen_print(const s8* str) {
	s8 c;
	s32 i = 0;
	while ((c = str[i]) != '\0') {
		++i;

		if (c == '\n') {
			// If the string has a \n, we artificially break the line
			screen.cursor_pos += VIDEO_COLS_NUM - (screen.cursor_pos % VIDEO_COLS_NUM);
		} else {
			// In the common case, we just print the character to the 
			s32 y = screen.cursor_pos / VIDEO_COLS_NUM;
			s32 x = screen.cursor_pos % VIDEO_COLS_NUM;
			print_at(c, y, x);
			++screen.cursor_pos;
		}

		// If we reached the end of the screen, we shift everything one line up
		if (screen.cursor_pos >= VIDEO_COLS_NUM * VIDEO_ROWS_NUM) {
			screen.cursor_pos -= VIDEO_COLS_NUM;
			shift_one_line_up();
		}
	}

	// Update the text cursor in the device
	update_text_cursor();
}

// Clear the screen and reset the cursor position
void screen_clear() {
	s8* video_memory = (s8*)VIDEO_MEMORY_ADDRESS;

	for (s32 i = 0; i < VIDEO_COLS_NUM * VIDEO_ROWS_NUM; ++i) {
		video_memory[2 * i] = 0;
		video_memory[2 * i + 1] = 0xF;
	}

	screen.cursor_pos = 0;
	update_text_cursor();
}
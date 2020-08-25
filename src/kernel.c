// reminder: once the kernel hits 512 bytes in size we will need to load more sectors.
// @TODO: fix this in QEMU

void print_to_screen(char* str) {
	// 80 x 25
	static int row = 0;
	char* video_memory = (char*) 0xb8000;
	int i = 0;
	while (str[i] != '\0') {
		video_memory[2 * row * 80 + i * 2] = str[i];
		video_memory[2 * row * 80 + i * 2 + 1] = 0x0F;
		++i;
	}
	++row;
}

void main() {
	for (int i = 0; i < 10; ++i) {
		print_to_screen("Welcome to the rawOS!");
	}
}

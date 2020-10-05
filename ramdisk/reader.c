#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "ramdisk.h"

s32 main(s32 argc, s8** argv) {
	s32 print_file_contents = 0;

	if (argc == 2 && strcmp(argv[1], "true") == 0) {
		print_file_contents = 1;
	}

	FILE* input_file = fopen(INITRD_OUTPUT_FILE, "rb");
	if (!input_file) {
		fprintf(stderr, "error opening output file %s: %s\n", INITRD_OUTPUT_FILE, strerror(errno));
		return 1;
	}
	u32 num_files;
	fread(&num_files, sizeof(u32), 1, input_file);

	Ramdisk_Header* headers = malloc(sizeof(Ramdisk_Header) * num_files);
	if (!headers) {
		fprintf(stderr, "error allocating memory to store headers: %s\n", strerror(errno));
		return 1;
	}
	fread(headers, sizeof(Ramdisk_Header), num_files, input_file);

	for (u32 i = 0; i < num_files; ++i) {
		printf("\tFile %u: %s (size: %u)\n", i + 1, headers[i].file_name, headers[i].file_size);
		if (print_file_contents) {
			s8* file_content = malloc(headers[i].file_size);
			if (!headers) {
				fprintf(stderr, "error allocating memory to store content of file %s: %s\n", headers[i].file_name, strerror(errno));
				return 1;
			}
			fread(file_content, headers[i].file_size, 1, input_file);
			printf("%s\n", file_content);
			free(file_content);
		}
	}

	free(headers);
	fclose(input_file);

	return 0;
}
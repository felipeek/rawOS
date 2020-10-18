#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "ramdisk.h"

/*
	initrd format:
	
	u32: number of files

	Ramdisk_Header: header 1
	Ramdisk_Header: header 2
	Ramdisk_Header: header 3
	Ramdisk_Header: header 4
	...
	Ramdisk_Header: header N (N = number of files)

	file1 size: file1 content
	file2 size: file2 content
	file3 size: file3 content
	file4 size: file4 content
	...
	fileN size: fileN content (N = number of files)
*/

static void print_usage(const s8* program_name) {
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "%s input_file_1 output_name_of_file_1_in_fs input_file_2 output_name_of_file_2_in_fs ... input_file_N output_name_of_file_N_in_fs\n", program_name);
	fprintf(stderr, "example: %s ./my_file /data/my_file ./my_other_file /data/my_other_file\n", program_name);
}

static s32 file_is_not_in_root_folder(const s8* file_name) {
	s32 i = 0;
	while (file_name[i] != 0) {
		if (file_name[i] == '/' || file_name[i] == '\\') {
			return 1;
		}
		++i;
	}

	return 0;
}

s32 main(s32 argc, s8** argv) {
	if (argc % 2 == 0 || argc == 1) {
		print_usage(argv[0]);
		return 1;
	}

	u32 num_files = (argc - 1) / 2;

	FILE* output_file = fopen(INITRD_OUTPUT_FILE, "wb");
	if (!output_file) {
		fprintf(stderr, "error opening output file %s: %s\n", INITRD_OUTPUT_FILE, strerror(errno));
		return 1;
	}

	fwrite(&num_files, sizeof(u32), 1, output_file);

	Ramdisk_Header* headers = malloc(sizeof(Ramdisk_Header) * num_files);

	for (u32 i = 0; i < num_files; ++i) {
		Ramdisk_Header current_header;

		FILE* file = fopen(argv[i * 2 + 1], "rb");
		if (!file) {
			fprintf(stderr, "error opening file %s: %s\n", argv[i * 2 + 1], strerror(errno));
			return 1;
		}
		fseek(file, 0, SEEK_END);
		current_header.file_size = ftell(file);
		strcpy(current_header.file_name, argv[i * 2 + 2]);
		fclose(file);

		if (file_is_not_in_root_folder(current_header.file_name)) {
			fprintf(stderr, "error: all files must be in the root folder (slashes are not allowed in the name)\n");
			return 1;
		}

		headers[i] = current_header;
	}

	fwrite(headers, sizeof(Ramdisk_Header), num_files, output_file);

	for (u32 i = 0; i < num_files; ++i) {
		FILE* file = fopen(argv[i * 2 + 1], "rb");
		if (!file) {
			fprintf(stderr, "error opening file %s: %s\n", argv[i * 2 + 1], strerror(errno));
			return 1;
		}
		s8* file_content = malloc(headers[i].file_size);
		if (!file_content) {
			fprintf(stderr, "error allocating memory to store contents of file %s: %s\n", argv[i * 2 + 1], strerror(errno));
			return 1;
		}
		fread(file_content, headers[i].file_size, 1, file);
		fclose(file);
		fwrite(file_content, headers[i].file_size, 1, output_file);
		free(file_content);
	}

	fclose(output_file);
	free(headers);
	return 0;
}
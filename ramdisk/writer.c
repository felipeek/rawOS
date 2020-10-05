#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "ramdisk.h"
#include "light_array.h"

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

static void print_usage(s8* program_name) {
	fprintf(stderr, "usage:\n");
	fprintf(stderr, "%s input_file_1 output_name_of_file_1_in_fs input_file_2 output_name_of_file_2_in_fs ... input_file_N output_name_of_file_N_in_fs\n", program_name);
	fprintf(stderr, "example: %s ./my_file /data/my_file ./my_other_file /data/my_other_file\n", program_name);
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

	Ramdisk_Header* headers = array_new(Ramdisk_Header);

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

		array_push(headers, current_header);
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
	array_free(headers);
	return 0;
}
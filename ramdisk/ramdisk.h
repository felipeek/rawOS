#ifndef RAW_OS_RAMDISK_RAMDISK_H
#define RAW_OS_RAMDISK_RAMDISK_H
#include "common.h"
#define INITRD_OUTPUT_FILE "./bin/initrd.img"
#define FILE_NAME_MAX 256

typedef struct {
	s8 file_name[FILE_NAME_MAX];
	u32 file_size;
} Ramdisk_Header;
#endif
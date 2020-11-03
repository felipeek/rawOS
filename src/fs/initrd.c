#include "initrd.h"
#include "../alloc/kalloc.h"
#include "../util/util.h"

extern u8 _initrd_data[]      asm("_binary_bin_initrd_img_start");
extern u8 _initrd_data_size[] asm("_binary_bin_initrd_img_size");
extern u8 _initrd_data_end[]  asm("_binary_bin_initrd_img_end");

typedef struct {
	s8 file_name[FILE_NAME_MAX_LENGTH];
	u32 file_size;
} Ramdisk_Header;

static u32 num_files;
static u8** initrd_files_data;
static Vfs_Node* initrd_root_node;
static Vfs_Node* initrd_files_nodes;

static s32 initrd_read(Vfs_Node* vfs_node, u32 offset, u32 size, void* buf) {
	if (offset >= vfs_node->size) {
		return 0;
	}
	u32 size_to_read = MIN(size, vfs_node->size - offset);
	u8* file_data = initrd_files_data[vfs_node->inode - 1];
	memcpy(buf, file_data + offset, size_to_read);
	return size_to_read;
}

static s32 initrd_readdir(Vfs_Node* vfs_node, u32 index, Vfs_Dirent* dirent) {
	assert(vfs_node == initrd_root_node, "initrd_readdir called for node that is not the root node");
	if (index >= num_files) {
		return -1;
	}
	strcpy(dirent->name, initrd_files_nodes[index].name);
	dirent->inode = initrd_files_nodes[index].inode;
	return 0;
}

static Vfs_Node* initrd_lookup(Vfs_Node* vfs_node, const s8* path) {
	assert(vfs_node == initrd_root_node, "initrd_lookup called for node that is not the root node");
	if (vfs_node == initrd_root_node) {
		for (u32 i = 0; i < num_files; ++i) {
			Vfs_Node* node = &initrd_files_nodes[i];
			if (!strcmp(path, node->name)) {
				return node;
			}
		}
	}
	return 0;
}

Vfs_Node* initrd_init() {
	u8* initrd_data = _initrd_data;

	num_files = *(u32*)initrd_data;
	initrd_data += sizeof(u32);

	Ramdisk_Header* headers = (Ramdisk_Header*)initrd_data;
	initrd_data += num_files * sizeof(Ramdisk_Header);

	initrd_files_nodes = kalloc_alloc(sizeof(Vfs_Node) * num_files);
	initrd_files_data = kalloc_alloc(sizeof(u8**) * num_files);

	for (u32 i = 0; i < num_files; ++i) {
		initrd_files_nodes[i].flags = VFS_FILE;
		strcpy(initrd_files_nodes[i].name, headers[i].file_name);
		initrd_files_nodes[i].close = 0;
		initrd_files_nodes[i].open = 0;
		initrd_files_nodes[i].read = initrd_read;
		initrd_files_nodes[i].write = 0;
		initrd_files_nodes[i].readdir = 0;
		initrd_files_nodes[i].lookup = 0;
		initrd_files_nodes[i].inode = i + 1;
		initrd_files_nodes[i].size = headers[i].file_size;

		initrd_files_data[i] = initrd_data;
		initrd_data += headers[i].file_size;
	}

	initrd_root_node = kalloc_alloc(sizeof(Vfs_Node));
	initrd_root_node->flags = VFS_DIRECTORY;
	strcpy(initrd_root_node->name, "initrd");
	initrd_root_node->close = 0;
	initrd_root_node->open = 0;
	initrd_root_node->read = 0;
	initrd_root_node->write = 0;
	initrd_root_node->readdir = initrd_readdir;
	initrd_root_node->lookup = initrd_lookup;
	initrd_root_node->inode = 0;
	initrd_root_node->size = 0;

	return initrd_root_node;
}
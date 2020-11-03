#include "dev.h"
#include "../screen.h"
#include "../util/util.h"
#include "../alloc/kalloc.h"

#define SCREEN_FILE_NAME "screen"

Vfs_Node* dev_root_node;
Vfs_Node* screen_node;

static s32 dev_read(Vfs_Node* vfs_node, u32 offset, u32 size, void* buf) {
	return 0;
}

static s32 dev_write(Vfs_Node* vfs_node, u32 offset, u32 size, void* buf) {
	if (vfs_node == screen_node) {
		screen_print_with_len(buf, size);
	}
	return 0;
}

static s32 dev_readdir(Vfs_Node* vfs_node, u32 index, Vfs_Dirent* dirent) {
	assert(vfs_node == dev_root_node, "dev_readdir called for node that is not the root node");
	if (index == 0) {
		strcpy(dirent->name, SCREEN_FILE_NAME);
		dirent->inode = 1;
		return 0;
	}
	return -1;
}

static Vfs_Node* dev_lookup(Vfs_Node* vfs_node, const s8* path) {
	if (vfs_node == dev_root_node) {
		if (!strcmp(path, SCREEN_FILE_NAME)) {
			return screen_node;
		}
	}
	return 0;
}

Vfs_Node* dev_init() {
	dev_root_node = kalloc_alloc(sizeof(Vfs_Node));
	dev_root_node->flags = VFS_DIRECTORY;
	strcpy(dev_root_node->name, "dev");
	dev_root_node->close = 0;
	dev_root_node->open = 0;
	dev_root_node->read = 0;
	dev_root_node->write = 0;
	dev_root_node->readdir = dev_readdir;
	dev_root_node->lookup = dev_lookup;
	dev_root_node->inode = 0;
	dev_root_node->size = 0;

	screen_node = kalloc_alloc(sizeof(Vfs_Node));
	screen_node->flags = VFS_FILE;
	strcpy(screen_node->name, SCREEN_FILE_NAME);
	screen_node->close = 0;
	screen_node->open = 0;
	screen_node->read = 0;
	screen_node->write = dev_write;
	screen_node->readdir = 0;
	screen_node->lookup = 0;
	screen_node->inode = 0;
	screen_node->size = 0;

	return dev_root_node;
}
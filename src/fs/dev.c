#include "dev.h"
#include "../screen.h"
#include "../util/util.h"
#include "../alloc/kalloc.h"
#include "../keyboard.h"
#include "../util/printf.h"
#include "../process.h"

#define SCREEN_FILE_NAME "screen"
#define KEYBOARD_FILE_NAME "keyboard"

Vfs_Node* dev_root_node;
Vfs_Node* screen_node;
Vfs_Node* keyboard_node;

static s32 dev_read(Vfs_Node* vfs_node, u32 offset, u32 size, void* buf) {
	if (vfs_node == keyboard_node) {
		Keyboard_Event_Receiver_Buffer* kerb = kalloc_alloc(sizeof(Keyboard_Event_Receiver_Buffer));
		kerb->buffer = kalloc_alloc(size);
		kerb->event_received = 0;
		kerb->buffer_filled = 0;
		kerb->buffer_capacity = size;
		kerb->pid = process_getpid();
		//printf("dev: registering %u\n", kerb->pid);
		keyboard_register_event_buffer(kerb);

		// spin-lock
		//printf("(1) buf: %x! [%x]\n", kerb->buffer, *(s8*)kerb->buffer);
		process_block_current();
		//printf("(2) buf: %x! [%x]\n", kerb->buffer, *(s8*)kerb->buffer);
		memcpy(buf, kerb->buffer, kerb->buffer_filled);
		u32 filled = kerb->buffer_filled;
		kalloc_free(kerb->buffer);
		kalloc_free(kerb);
		
		return filled;
	}
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
	} else if (index == 1) {
		strcpy(dirent->name, KEYBOARD_FILE_NAME);
		dirent->inode = 2;
		return 0;
	}
	return -1;
}

static Vfs_Node* dev_lookup(Vfs_Node* vfs_node, const s8* path) {
	if (vfs_node == dev_root_node) {
		if (!strcmp(path, SCREEN_FILE_NAME)) {
			return screen_node;
		} else if (!strcmp(path, KEYBOARD_FILE_NAME)) {
			return keyboard_node;
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

	keyboard_node = kalloc_alloc(sizeof(Vfs_Node));
	keyboard_node->flags = VFS_FILE;
	strcpy(keyboard_node->name, KEYBOARD_FILE_NAME);
	keyboard_node->close = 0;
	keyboard_node->open = 0;
	keyboard_node->read = dev_read;
	keyboard_node->write = 0;
	keyboard_node->readdir = 0;
	keyboard_node->lookup = 0;
	keyboard_node->inode = 0;
	keyboard_node->size = 0;

	return dev_root_node;
}
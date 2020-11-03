#include "vfs.h"
#include "initrd.h"
#include "dev.h"
#include "../util/util.h"
#include "../alloc/kalloc.h"

#define NUM_ROOT_NODES 2
Vfs_Node* vfs_root = 0;
static Vfs_Node* root_nodes[NUM_ROOT_NODES];

static s32 vfs_root_readdir(Vfs_Node* vfs_node, u32 index, Vfs_Dirent* dirent) {
	assert(vfs_node == vfs_root, "vfs_root_readdir called for node that is not the root node");
	if (index >= NUM_ROOT_NODES) {
		return -1;
	}
	strcpy(dirent->name, root_nodes[index]->name);
	dirent->inode = root_nodes[index]->inode;
	return 0;
}

static Vfs_Node* vfs_root_lookup(Vfs_Node* vfs_node, const s8* path) {
	assert(vfs_node == vfs_root, "vfs_root_lookup called for node that is not the root node");
	if (vfs_node == vfs_root) {
		for (u32 i = 0; i < NUM_ROOT_NODES; ++i) {
			Vfs_Node* node = root_nodes[i];
			if (!strcmp(path, node->name)) {
				return node;
			}
		}
	}
	return 0;
}

void vfs_init() {
	vfs_root = kalloc_alloc(sizeof(Vfs_Node));
	vfs_root->flags = VFS_DIRECTORY;
	strcpy(vfs_root->name, "");
	vfs_root->close = 0;
	vfs_root->open = 0;
	vfs_root->read = 0;
	vfs_root->write = 0;
	vfs_root->readdir = vfs_root_readdir;
	vfs_root->lookup = vfs_root_lookup;
	vfs_root->inode = 0;
	vfs_root->size = 0;

	root_nodes[0] = initrd_init();
	root_nodes[1] = dev_init();
}

void vfs_close(Vfs_Node* vfs_node) {
	if (vfs_node->close) {
		vfs_node->close(vfs_node);
	}
}

void vfs_open(Vfs_Node* vfs_node, u32 flags) {
	if (vfs_node->open) {
		vfs_node->open(vfs_node, flags);
	}
}

s32 vfs_read(Vfs_Node* vfs_node, u32 offset, u32 size, void* buf) {
	if (vfs_node->read) {
		return vfs_node->read(vfs_node, offset, size, buf);
	}
	return 0;
}

s32 vfs_write(Vfs_Node* vfs_node, u32 offset, u32 size, void* buf) {
	if (vfs_node->write) {
		return vfs_node->write(vfs_node, offset, size, buf);
	}
	return 0;
}

s32 vfs_readdir(Vfs_Node* vfs_node, u32 index, Vfs_Dirent* dirent) {
	if (vfs_node->readdir) {
		return vfs_node->readdir(vfs_node, index, dirent);
	}
	return 0;
}

Vfs_Node* vfs_lookup(Vfs_Node* vfs_node, const s8* path) {
	if (vfs_node->lookup) {
		return vfs_node->lookup(vfs_node, path);
	}
	return 0;
}
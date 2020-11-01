#include "vfs.h"

Vfs_Node* vfs_root = 0;

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
#ifndef RAW_OS_FS_VFS_H
#define RAW_OS_FS_VFS_H
#include "../common.h"
#define FILE_NAME_MAX_LENGTH 256

#define VFS_FILE 0x01
#define VFS_DIRECTORY 0x02

struct Vfs_Node;
struct Vfs_Dirent;

// https://pubs.opengroup.org/onlinepubs/009695399/functions/read.html
typedef s32 (*Vfs_Read)(struct Vfs_Node* vfs_node, u32 offset, u32 size, void* buf);
// https://pubs.opengroup.org/onlinepubs/009695399/functions/write.html
typedef s32 (*Vfs_Write)(struct Vfs_Node* vfs_node, u32 offset, u32 size, void* buf);
// https://pubs.opengroup.org/onlinepubs/009695399/functions/open.html
typedef void (*Vfs_Open)(struct Vfs_Node* vfs_node, u32 flags);
// https://pubs.opengroup.org/onlinepubs/009695399/functions/close.html
typedef void (*Vfs_Close)(struct Vfs_Node* vfs_node);
// https://pubs.opengroup.org/onlinepubs/009695399/functions/readdir.html
typedef s32 (*Vfs_Readdir)(struct Vfs_Node* vfs_node, u32 index, struct Vfs_Dirent* dirent);
typedef struct Vfs_Node* (*Vfs_Lookup)(struct Vfs_Node* vfs_node, const s8* path);

typedef struct Vfs_Dirent {
	s8 name[FILE_NAME_MAX_LENGTH];
	u32 inode;
} Vfs_Dirent;

typedef struct Vfs_Node {
	s8 name[FILE_NAME_MAX_LENGTH];
	u32 flags;
	u32 inode;
	u32 size;
	Vfs_Read read;
	Vfs_Write write;
	Vfs_Open open;
	Vfs_Close close;
	Vfs_Readdir readdir;
	Vfs_Lookup lookup;
} Vfs_Node;

// For now, we are delegating to external modules the initialization of the vfs_root.
extern Vfs_Node* vfs_root;

void vfs_close(Vfs_Node* vfs_node);
void vfs_open(Vfs_Node* vfs_node, u32 flags);
s32 vfs_read(Vfs_Node* vfs_node, u32 offset, u32 size, void* buf);
s32 vfs_write(Vfs_Node* vfs_node, u32 offset, u32 size, void* buf);
s32 vfs_readdir(Vfs_Node* vfs_node, u32 index, Vfs_Dirent* dirent);
Vfs_Node* vfs_lookup(Vfs_Node* vfs_node, const s8* path);

#endif
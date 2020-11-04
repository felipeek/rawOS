#include "util.h"
#include "../util/util.h"

Vfs_Node* fs_util_get_node_by_path(const s8* path) {
	Vfs_Node* result = vfs_root;
	s8 filename_buffer[VFS_FILE_NAME_MAX_LENGTH];

	assert(path[0] == '/', "Only absolute paths are supported for now. Path needs to start with slash (/), but got %s.", path);

	u32 path_position = 1;
	u32 filename_buffer_position = 0;
	while (path[path_position] != 0) {
		if (path[path_position] == '/') {
			filename_buffer[filename_buffer_position] = '\0';
			result = vfs_lookup(result, filename_buffer);
			if (!result) {
				return 0;
			}
			filename_buffer_position = 0;
		} else {
			filename_buffer[filename_buffer_position++] = path[path_position];
		}
		++path_position;
	}

	if (filename_buffer_position != 0) {
		filename_buffer[filename_buffer_position] = '\0';
		result = vfs_lookup(result, filename_buffer);
	}

	return result;
}
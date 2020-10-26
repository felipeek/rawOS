#ifndef RAW_OS_ALLOC_KALLOC_AVL_H
#define RAW_OS_ALLOC_KALLOC_AVL_H
#include "../common.h"

typedef struct Kalloc_AVL_Node {
	struct Kalloc_AVL_Node* left;
	struct Kalloc_AVL_Node* right;
	s32 height;						// The height of this node, in the AVL
	u32 hole_size;					// The size of the hole, in bytes
	void* hole_addr;				// The address of the hole, pointing to the Heap Header
} Kalloc_AVL_Node;

typedef struct {
	Kalloc_AVL_Node* free;
	Kalloc_AVL_Node* root;
} Kalloc_AVL;

void kalloc_avl_init(Kalloc_AVL* avl, void* data, u32 data_size);
void* kalloc_avl_find_hole(const Kalloc_AVL* avl, u32 hole_size, u32 alignment);
s32 kalloc_avl_insert(Kalloc_AVL* avl, u32 hole_size, void* hole_addr);
s32 kalloc_avl_remove(Kalloc_AVL* avl, u32 hole_size, void* hole_addr);

#endif
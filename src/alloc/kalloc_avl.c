#include "kalloc_avl.h"
#include "../util/util.h"

// Compare the size of two holes.
static s32 compare_hole_size(u32 s1, u32 s2) {
	if (s1 > s2) return 1;
	if (s1 < s2) return -1;
	return 0;
}

// Compare two holes, taking into consideration their addresses too.
static s32 compare_hole(u32 h1_size, void* h1_addr, u32 h2_size, void* h2_addr) {
	if (h1_size > h2_size) return 1;
	if (h1_size < h2_size) return -1;
	if (h1_addr > h2_addr) return 1;
	if (h1_addr < h2_addr) return -1;
	return 0;
}

void kalloc_avl_init(Kalloc_AVL* avl, void* data, u32 data_size) {
	util_assert(data_size >= sizeof(Kalloc_AVL_Node), "kalloc_avl: can't initiate. data_size must be greater or equal to the size of the node");
	u32 num_nodes = data_size / sizeof(Kalloc_AVL_Node);
	for (u32 i = 0; i < num_nodes; ++i) {
		Kalloc_AVL_Node* previous = (i > 0) ? (Kalloc_AVL_Node*)data + i - 1 : 0;
		Kalloc_AVL_Node* current = (Kalloc_AVL_Node*)data + i;
		Kalloc_AVL_Node* next = (i != num_nodes - 1) ? (Kalloc_AVL_Node*)data + i + 1: 0;
		current->left = previous;
		current->right = next;
		current->hole_size = 0;
		current->hole_addr = 0;
		current->height = 0;
	}

	avl->root = 0;
	avl->free = (Kalloc_AVL_Node*)data;
}

static Kalloc_AVL_Node* get_free_node(Kalloc_AVL* avl) {
	Kalloc_AVL_Node* target = avl->free;
	util_assert(target != 0, "kalloc_avl: no free node available");
	if (avl->free->right) {
		util_assert(avl->free->right->left == target, "kalloc_avl: next node is not pointing to current");
		avl->free->right->left = 0;
	}
	avl->free = avl->free->right;
	return target;
}

static void put_free_node(Kalloc_AVL* avl, Kalloc_AVL_Node* node) {
	Kalloc_AVL_Node* previous_root = avl->free;
	avl->free = node;
	avl->free->left = 0;
	avl->free->right = previous_root;
	if (previous_root) {
		util_assert(previous_root->left == 0, "kalloc_avl: previous root free node had something on the left");
		previous_root->left = avl->free;
	}
}

static int hole_has_alignment_space(const Kalloc_AVL_Node* node, u32 wanted_size, u32 alignment) {
	u32 aligned_addr = (u32)node->hole_addr;
	if (alignment != 0) {
		if (aligned_addr & (alignment - 1)) {
			aligned_addr &= ~(alignment - 1);
			aligned_addr += alignment;
		}
	}

	return node->hole_size >= wanted_size + (aligned_addr - (u32)node->hole_addr);
}

static void* avl_find_hole_internal(Kalloc_AVL_Node* node, u32 hole_size, u32 alignment) {
	if (!node) {
		return 0;
	}

	s32 comparison = compare_hole_size(hole_size, node->hole_size);
	if (comparison > 0 || !hole_has_alignment_space(node, hole_size, alignment)) {
		return avl_find_hole_internal(node->right, hole_size, alignment);
	} else {
		// When comparison == 0, we let the code fall here aswell.
		// This will ensure that we will always choose the smallest hole address
		// Which is useful to prevent fragmentation and allow heap shrinking
		void* hole_addr = avl_find_hole_internal(node->left, hole_size, alignment);
		return hole_addr ? hole_addr : node->hole_addr;
	}
}

// Finds a hole whose size is equal or greater than hole_size.
// The chosen hole will always be the smallest one possible.
// If there are more than 1 candidate holes, we pick the one with smallest addr.
// @TODO(fek): when we introduce heap shrinking, we might want to give preference to picking the smallest addr, instead of smallest size.
// This would improve the shrinking, but it could increase the fragmentation
void* kalloc_avl_find_hole(const Kalloc_AVL* avl, u32 hole_size, u32 alignment) {
	return avl_find_hole_internal(avl->root, hole_size, alignment);
}

static void update_node_height(Kalloc_AVL_Node* node) {
	s32 left_height = node->left ? node->left->height : 0;
	s32 right_height = node->right ? node->right->height : 0;
	node->height = MAX(right_height, left_height) + 1;
}

static Kalloc_AVL_Node* rotate_right(Kalloc_AVL_Node* root) {
	Kalloc_AVL_Node* pivot = root->left;
	Kalloc_AVL_Node* child = pivot->left;

	root->left = pivot->right;
	pivot->right = root;

	update_node_height(root);
	update_node_height(pivot);

	return pivot;
}

static Kalloc_AVL_Node* rotate_left(Kalloc_AVL_Node* root) {
	Kalloc_AVL_Node* pivot = root->right;
	Kalloc_AVL_Node* child = pivot->right;

	root->right = pivot->left;
	pivot->left = root;

	update_node_height(root);
	update_node_height(pivot);

	return pivot;
}

static Kalloc_AVL_Node* double_rotate_right(Kalloc_AVL_Node* root) {
	Kalloc_AVL_Node* top = root;
	Kalloc_AVL_Node* middle = root->left;
	Kalloc_AVL_Node* bottom = middle->right;

	middle->right = bottom->left;
	top->left = bottom->right;
	bottom->left = middle;
	bottom->right = top;

	update_node_height(top);
	update_node_height(middle);
	update_node_height(bottom);

	return bottom;
}

static Kalloc_AVL_Node* double_rotate_left(Kalloc_AVL_Node* root) {
	Kalloc_AVL_Node* top = root;
	Kalloc_AVL_Node* middle = root->right;
	Kalloc_AVL_Node* bottom = middle->left;

	middle->left = bottom->right;
	top->right = bottom->left;
	bottom->left = top;
	bottom->right = middle;

	update_node_height(top);
	update_node_height(middle);
	update_node_height(bottom);

	return bottom;
}

static s32 get_balance_factor(const Kalloc_AVL_Node* node) {
	Kalloc_AVL_Node* left = node->left;
	Kalloc_AVL_Node* right = node->right;

	s32 left_height = left ? left->height : 0;
	s32 right_height = right ? right->height : 0;

	return right_height - left_height;
}

static Kalloc_AVL_Node* insert_internal(Kalloc_AVL* avl, Kalloc_AVL_Node* avl_node, u32 hole_size, void* hole_addr, s32* subtree_height_changed, s32* error) {
	if (!avl_node) {
		avl_node = get_free_node(avl);
		if (!avl_node) {
			*error = 1;
			return 0;
		} else {
			avl_node->hole_addr = hole_addr;
			avl_node->hole_size = hole_size;
			avl_node->left = 0;
			avl_node->right = 0;
			avl_node->height = 1;
			*subtree_height_changed = 1;
			return avl_node;
		}
	}

	Kalloc_AVL_Node* new_root = avl_node;
	s32 comparison = compare_hole(hole_size, hole_addr, avl_node->hole_size, avl_node->hole_addr);
	util_assert(comparison != 0, "heap_avl: Trying to insert same element to AVL.");
	if (comparison > 0) {
		avl_node->right = insert_internal(avl, avl_node->right, hole_size, hole_addr, subtree_height_changed, error);
		if (*subtree_height_changed) {
			s32 balance_factor = get_balance_factor(avl_node);
			util_assert(balance_factor >= 0 && balance_factor <= 2, "heap_avl: balance_factor must be between 0 and 2, but got %u", balance_factor);
			if (balance_factor == 0) {
				*subtree_height_changed = 0;
			} else if (balance_factor == 1) {
				*subtree_height_changed = 1;
				++avl_node->height;
			} else if (balance_factor == 2) {
				s32 right_child_balance_factor = get_balance_factor(avl_node->right);
				if (right_child_balance_factor == -1) {
					new_root = double_rotate_left(avl_node);
				} else {
					new_root = rotate_left(avl_node);
				}
				*subtree_height_changed = 0;
			}
		}
	} else if (comparison < 0) {
		avl_node->left = insert_internal(avl, avl_node->left, hole_size, hole_addr, subtree_height_changed, error);
		if (*subtree_height_changed) {
			s32 balance_factor = get_balance_factor(avl_node);
			util_assert(balance_factor >= -2 && balance_factor <= 0, "heap_avl: balance_factor must be between -2 and 0, but got %u", balance_factor);
			if (balance_factor == 0) {
				*subtree_height_changed = 0;
			} else if (balance_factor == -1) {
				*subtree_height_changed = 1;
				++avl_node->height;
			} else if (balance_factor == -2) {
				s32 left_child_balance_factor = get_balance_factor(avl_node->left);
				if (left_child_balance_factor == 1) {
					new_root = double_rotate_right(avl_node);
				} else {
					new_root = rotate_right(avl_node);
				}
				*subtree_height_changed = 0;
			}
		}
	}

	return new_root;
}

// Insert a hole to the AVL.
s32 kalloc_avl_insert(Kalloc_AVL* avl, u32 hole_size, void* hole_addr) {
	s32 subtree_height_changed, error = 0;
	avl->root = insert_internal(avl, avl->root, hole_size, hole_addr, &subtree_height_changed, &error);
	return error;
}

static Kalloc_AVL_Node* find_predecessor(Kalloc_AVL_Node* node) {
	Kalloc_AVL_Node* current = node->left;
	while (current->right) {
		current = current->right;
	}
	return current;
}

static Kalloc_AVL_Node* remove_internal(Kalloc_AVL* avl, Kalloc_AVL_Node* avl_node, u32 hole_size, void* hole_addr, s32* subtree_height_changed, s32* not_found) {
	if (!avl_node) {
		*not_found = 1;
		return 0;
	}

	Kalloc_AVL_Node* new_root = avl_node;
	s32 comparison = compare_hole(hole_size, hole_addr, avl_node->hole_size, avl_node->hole_addr);

	if (comparison == 0) {
		if (!avl_node->left && !avl_node->right) {
			put_free_node(avl, avl_node);
			*subtree_height_changed = 1;
			return 0;
		} else if (!avl_node->left) {
			avl_node->hole_size = avl_node->right->hole_size;
			avl_node->hole_addr = avl_node->right->hole_addr;
			put_free_node(avl, avl_node->right);
			avl_node->right = 0;
			--avl_node->height;
			*subtree_height_changed = 1;
			return avl_node;
		} else if (!avl_node->right) {
			avl_node->hole_size = avl_node->left->hole_size;
			avl_node->hole_addr = avl_node->left->hole_addr;
			put_free_node(avl, avl_node->left);
			avl_node->left = 0;
			--avl_node->height;
			*subtree_height_changed = 1;
			return avl_node;
		} else {
			Kalloc_AVL_Node* predecessor = find_predecessor(avl_node);
			comparison = compare_hole(predecessor->hole_size, predecessor->hole_addr, avl_node->hole_size, avl_node->hole_addr);
			hole_size = predecessor->hole_size;
			hole_addr = predecessor->hole_addr;
			avl_node->hole_size = predecessor->hole_size;
			avl_node->hole_addr = predecessor->hole_addr;
		}
	}
	
	if (comparison > 0) {
		avl_node->right = remove_internal(avl, avl_node->right, hole_size, hole_addr, subtree_height_changed, not_found);
		if (*subtree_height_changed) {
			s32 balance_factor = get_balance_factor(avl_node);
			util_assert(balance_factor >= -2 && balance_factor <= 0, "heap_avl: balance_factor must be between -2 and 0, but got %u", balance_factor);
			if (balance_factor == 0) {
				*subtree_height_changed = 1;
				--avl_node->height;
			} else if (balance_factor == -1) {
				*subtree_height_changed = 0;
			} else if (balance_factor == -2) {
				s32 left_child_balance_factor = get_balance_factor(avl_node->left);
				if (left_child_balance_factor == 1) {
					new_root = double_rotate_right(avl_node);
				} else {
					new_root = rotate_right(avl_node);
				}

				if (get_balance_factor(new_root) == 1) {
					*subtree_height_changed = 0;
				} else {
					*subtree_height_changed = 1;
				}
			}
		}
	} else {
		avl_node->left = remove_internal(avl, avl_node->left, hole_size, hole_addr, subtree_height_changed, not_found);
		if (*subtree_height_changed) {
			s32 balance_factor = get_balance_factor(avl_node);
			util_assert(balance_factor >= 0 && balance_factor <= 2, "heap_avl: balance_factor must be between 0 and 2, but got %u", balance_factor);
			if (balance_factor == 0) {
				*subtree_height_changed = 1;
				--avl_node->height;
			} else if (balance_factor == 1) {
				*subtree_height_changed = 0;
			} else if (balance_factor == 2) {
				s32 right_child_balance_factor = get_balance_factor(avl_node->right);
				if (right_child_balance_factor == -1) {
					new_root = double_rotate_left(avl_node);
				} else {
					new_root = rotate_left(avl_node);
				}
				
				if (get_balance_factor(new_root) == -1) {
					*subtree_height_changed = 0;
				} else {
					*subtree_height_changed = 1;
				}
			}
		}
	}

	return new_root;
}

// Remove a hole from the AVL.
s32 kalloc_avl_remove(Kalloc_AVL* avl, u32 hole_size, void* hole_addr) {
	s32 subtree_height_changed, not_found = 0;
	avl->root = remove_internal(avl, avl->root, hole_size, hole_addr, &subtree_height_changed, &not_found);
	return not_found;
}
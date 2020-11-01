#ifndef C_HASH_MAP_H
#define C_HASH_MAP_H

#include "common.h"
#include "util/util.h"
#include "alloc/kalloc.h"
#include "util/printf.h"

// Compares two keys. Needs to return 1 if the keys are equal, 0 otherwise.
typedef s32 (*Key_Compare_Func)(const void *key1, const void *key2);
// Calculates the hash of the key.
typedef u32 (*Key_Hash_Func)(const void *key);
// Called for every key-value pair. 'key and 'value' contain the key and the value.
// 'custom_data' contains the custom data sent in the 'hash_map_for_each_entry' call.
typedef void (*For_Each_Func)(const void *key, const void* value, void* custom_data);
// Do not change the Hash_Map struct
typedef struct {
    u32 capacity;
    u32 num_elements;
    u32 key_size;
    u32 value_size;
    Key_Compare_Func key_compare_func;
    Key_Hash_Func key_hash_func;
    void *data;
} Hash_Map;
// Creates a hash map. 'initial_capacity' indicates the initial capacity of the hash_map, in number of elements.
// 'key_compare_func' and 'key_hash_func' should be provided by the caller.
// Returns 0 if sucess, -1 otherwise.
s32 hash_map_create(Hash_Map *hm, u32 initial_capacity, u32 key_size, u32 value_size,
                    Key_Compare_Func key_compare_func, Key_Hash_Func key_hash_func);
// Put an element in the hash map.
// Returns 0 if sucess, -1 otherwise.
s32 hash_map_put(Hash_Map *hm, const void *key, const void *value);
// Get an element from the hash map. Note that the received element is a copy and not the actual element in the hash map.
// Returns 0 if sucess, -1 otherwise.
s32 hash_map_get(Hash_Map *hm, const void *key, void *value);
// Delete an element from the hash map.
// Returns 0 if sucess, -1 otherwise.
s32 hash_map_delete(Hash_Map *hm, const void *key);
// Destroys the hashmap, freeing the memory.
void hash_map_destroy(Hash_Map *hm);
// Calls a custom function for every entry in the hash map. The function 'for_each_func' must be provided.
// 'custom_data' is a custom pos32er that is repassed in every call.
// NOTE: Do not put or delete elements to the same hash table inside the for_each_func.
// Putting or deleting elements might alter the hash table s32ernal data structure, which will cause unexpected behavior.
void hash_map_for_each_entry(Hash_Map *hm, For_Each_Func for_each_func, void *custom_data);

#ifdef HASH_MAP_IMPLEMENT

typedef struct {
    s32 valid;
} Hash_Map_Element_Information;

static Hash_Map_Element_Information *get_element_information(Hash_Map *hm, u32 index) {
    return (Hash_Map_Element_Information *)((u8 *)hm->data +
                                            index * (sizeof(Hash_Map_Element_Information) + hm->key_size + hm->value_size));
}

static void *get_element_key(Hash_Map *hm, u32 index) {
    Hash_Map_Element_Information *hmei = get_element_information(hm, index);
    return (u8 *)hmei + sizeof(Hash_Map_Element_Information);
}

static void *get_element_value(Hash_Map *hm, u32 index) {
    Hash_Map_Element_Information *hmei = get_element_information(hm, index);
    return (u8 *)hmei + sizeof(Hash_Map_Element_Information) + hm->key_size;
}

static void put_element_key(Hash_Map *hm, u32 index, const void *key) {
    void *target = get_element_key(hm, index);
    util_memcpy(target, key, hm->key_size);
}

static void put_element_value(Hash_Map *hm, u32 index, const void *value) {
    void *target = get_element_value(hm, index);
    util_memcpy(target, value, hm->value_size);
}

s32 hash_map_create(Hash_Map *hm, u32 initial_capacity, u32 key_size, u32 value_size,
                    Key_Compare_Func key_compare_func, Key_Hash_Func key_hash_func) {
    hm->key_compare_func = key_compare_func;
    hm->key_hash_func = key_hash_func;
    hm->key_size = key_size;
    hm->value_size = value_size;
    hm->capacity = initial_capacity > 0 ? initial_capacity : 1;
    hm->num_elements = 0;
    hm->data = kalloc_alloc(hm->capacity * (sizeof(Hash_Map_Element_Information) + key_size + value_size));
	util_memset(hm->data, 0, hm->capacity * (sizeof(Hash_Map_Element_Information) + key_size + value_size));
    if (!hm->data) {
        return -1;
    }
    return 0;
}

void hash_map_destroy(Hash_Map *hm) {
    kalloc_free(hm->data);
}

static s32 hash_map_grow(Hash_Map *hm) {
    Hash_Map old_hm = *hm;
    if (hash_map_create(hm, old_hm.capacity << 1, old_hm.key_size, old_hm.value_size, old_hm.key_compare_func, old_hm.key_hash_func)) {
        return -1;
    }
    for (u32 pos = 0; pos < old_hm.capacity; ++pos) {
        Hash_Map_Element_Information *hmei = get_element_information(&old_hm, pos);
        if (hmei->valid) {
            void *key = get_element_key(&old_hm, pos);
            void *value = get_element_value(&old_hm, pos);
            if (hash_map_put(hm, key, value))
                return -1;
        }
    }
    hash_map_destroy(&old_hm);
    return 0;
}

s32 hash_map_put(Hash_Map *hm, const void *key, const void *value) {
    u32 pos = hm->key_hash_func(key) % hm->capacity;
    for (;;) {
        Hash_Map_Element_Information *hmei = get_element_information(hm, pos);
        if (!hmei->valid) {
            hmei->valid = 1;
            put_element_key(hm, pos, key);
            put_element_value(hm, pos, value);
            ++hm->num_elements;
            break;
        } else {
            void *element_key = get_element_key(hm, pos);
            if (hm->key_compare_func(element_key, key)) {
                put_element_key(hm, pos, key);
                put_element_value(hm, pos, value);
                break;
            }
        }
        pos = (pos + 1) % hm->capacity;
    }
    if ((hm->num_elements << 1) > hm->capacity) {
        if (hash_map_grow(hm)) {
            return -1;
        }
    }
    return 0;
}

s32 hash_map_get(Hash_Map *hm, const void *key, void *value) {
    u32 pos = hm->key_hash_func(key) % hm->capacity;
    for (;;) {
        Hash_Map_Element_Information *hmei = get_element_information(hm, pos);
        if (hmei->valid) {
            void *possible_key = get_element_key(hm, pos);
            if (hm->key_compare_func(possible_key, key)) {
                void *entry_value = get_element_value(hm, pos);
                util_memcpy(value, entry_value, hm->value_size);
                return 0;
            }
        } else {
            return -1;
        }
        pos = (pos + 1) % hm->capacity;
    }
}

static void adjust_gap(Hash_Map *hm, u32 gap_index) {
    u32 pos = (gap_index + 1) % hm->capacity;
    for (;;) {
        Hash_Map_Element_Information *current_hmei = get_element_information(hm, pos);
        if (!current_hmei->valid) {
            break;
        }
        void *current_key = get_element_key(hm, pos);
        u32 hash_position = hm->key_hash_func(current_key) % hm->capacity;
        u32 normalized_gap_index = (gap_index < hash_position) ? gap_index + hm->capacity : gap_index;
        u32 normalized_pos = (pos < hash_position) ? pos + hm->capacity : pos;
        if (normalized_gap_index >= hash_position && normalized_gap_index <= normalized_pos) {
            void *current_value = get_element_value(hm, pos);
            current_hmei->valid = 0;
            Hash_Map_Element_Information *gap_hmei = get_element_information(hm, gap_index);
            put_element_key(hm, gap_index, current_key);
            put_element_value(hm, gap_index, current_value);
            gap_hmei->valid = 1;
            gap_index = pos;
        }
        pos = (pos + 1) % hm->capacity;
    }
}

s32 hash_map_delete(Hash_Map *hm, const void *key) {
    u32 pos = hm->key_hash_func(key) % hm->capacity;
    for (;;) {
        Hash_Map_Element_Information *hmei = get_element_information(hm, pos);
        if (hmei->valid) {
            void *possible_key = get_element_key(hm, pos);
            if (hm->key_compare_func(possible_key, key)) {
                hmei->valid = 0;
                adjust_gap(hm, pos);
                --hm->num_elements;
                return 0;
            }
        } else {
            return -1;
        }
        pos = (pos + 1) % hm->capacity;
    }
}

void hash_map_for_each_entry(Hash_Map *hm, For_Each_Func for_each_func, void *custom_data) {
    for (u32 pos = 0; pos < hm->capacity; ++pos) {
        Hash_Map_Element_Information *hmei = get_element_information(hm, pos);
        if (hmei->valid) {
            void *key = get_element_key(hm, pos);
            void *value = get_element_value(hm, pos);
            for_each_func(key, value, custom_data);
        }
    }
}
#endif
#endif
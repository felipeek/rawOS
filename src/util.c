#include "util.h"

void util_memcpy(void* dst, void* src, u32 size) {
    for (u32 i = 0; i < size; ++i) {
        *((s8*)dst + i) = *((s8*)src + i);
    }
}
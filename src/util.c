#include "util.h"

void util_memcpy(void* dst, void* src, unsigned int size) {
    for (unsigned int i = 0; i < size; ++i) {
        *((char*)dst + i) = *((char*)src + i);
    }
}
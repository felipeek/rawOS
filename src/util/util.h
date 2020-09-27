#ifndef RAW_OS_UTIL_UTIL_H
#include "../common.h"
void util_memcpy(void* dst, void* src, u32 size);
void util_memset(void* ptr, u8 value, u32 num);
void util_assert(const s8* message, int condition);
void util_panic(const s8* message);
#endif
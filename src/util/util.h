#ifndef RAW_OS_UTIL_UTIL_H
#include "../common.h"
void util_strcpy(s8* dst, const s8* src);
s32 util_strcmp(const s8* s1, const s8* s2);
void util_memcpy(void* dst, const void* src, u32 size);
void util_memset(void* ptr, u8 value, u32 num);
void util_assert(const s8* message, int condition);
void util_panic(const s8* message);
#endif
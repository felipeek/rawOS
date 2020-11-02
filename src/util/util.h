#ifndef RAW_OS_UTIL_UTIL_H
#include "../common.h"
void strcpy(s8* dst, const s8* src);
s32 strlen(s8* str);
s32 strcmp(const s8* s1, const s8* s2);
void memcpy(void* dst, const void* src, u32 size);
void memset(void* ptr, u8 value, u32 num);
void assert(s32 condition, const s8* message, ...);
void panic(const s8* message);
#endif
#ifndef RAW_OS_UTIL_PRINTF_H
#define RAW_OS_UTIL_PRINTF_H
#include "../common.h"
void printf(const s8* str, ...);
void vprintf(const s8 *str, rawos_va_list args);
#endif
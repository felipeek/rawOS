#ifndef RAW_OS_COMMON_H
#define RAW_OS_COMMON_H
#define MAX(x, y) ((x > y) ? x : y)
#define MIN(x, y) ((x < y) ? x : y)
typedef double r64;
typedef float r32;
typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef short s16;
typedef unsigned char u8;
typedef char s8;

// variadic arguments
#define _INTSIZEOF(n) ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define _ADDRESSOF(v) (&(v))
#define rawos_va_arg(ap, t) (*(t*)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
#define rawos_va_start(ap, v) ((void)(ap = (rawos_va_list)_ADDRESSOF(v) + _INTSIZEOF(v)))
#define rawos_va_end(ap) ((void)(ap = (rawos_va_list)0))
typedef char* rawos_va_list;

#define OFFSETOF(st, m) ((u32)&(((st *)0)->m))

#endif
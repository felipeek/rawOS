#ifndef RAW_OS_INTERRUPT_H
#define RAW_OS_INTERRUPT_H
#include "common.h"

#define IDT_BASE 0x00000000
#define IDT_SIZE 256

typedef struct __attribute__((packed)) {
    u16 offset_low;
    u16 selector;
    u8 always0;
    u8 flags;
    u16 offset_high;
} IDT_Descriptor;

typedef struct __attribute__((packed)) {
    IDT_Descriptor descriptors[IDT_SIZE];
    u16 limit;
    u32 base;
} IDT;

void idt_init();
#endif
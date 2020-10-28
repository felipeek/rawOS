#ifndef RAW_OS_PROCESS_H
#define RAW_OS_PROCESS_H
#include "common.h"
void process_init();
s32 process_fork();
void process_switch();
#endif
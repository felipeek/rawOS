#ifndef RAW_OS_ASM_IO_H
#define RAW_OS_ASM_IO_H
#include "../common.h"
u8 io_port_byte_in(u16 port);
void io_port_byte_out(u16 port, u8 data);
u8 io_byte_in(u16 port);
void io_byte_out(u16 port, u8 data);
#endif
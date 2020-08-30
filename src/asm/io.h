#ifndef RAW_OS_ASM_IO_H
#define RAW_OS_ASM_IO_H
unsigned char io_port_byte_in(unsigned short port);
void io_port_byte_out(unsigned short port, unsigned char data);
unsigned char io_byte_in(unsigned short port);
void io_byte_out(unsigned short port, unsigned char data);
#endif
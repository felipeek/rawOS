#include "syscall.h"
#include "interrupt.h"
#include "util/printf.h"

static void syscall_handler(const Interrupt_Handler_Args* args) {
	printf("syscall called!\n");
}

void syscall_init() {
	interrupt_register_handler(syscall_handler, ISR128);
}

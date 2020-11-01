#include "timer.h"
#include "asm/io.h"
#include "util/printf.h"
#include "interrupt.h"
#include "process.h"

#define PIT_CLOCK_FREQUENCY_HZ 1193180
#define PIT_DATA_PORT_0 0x40
#define PIT_DATA_PORT_1 0x41
#define PIT_DATA_PORT_2 0x42
#define PIT_COMMAND_PORT 0x43
#define TIMER_DESIRED_FREQUENCY_HZ 100

static void timer_interrupt_handler(Interrupt_Handler_Args* args) {
	static u32 tick = 0;
	tick++;
	if (tick % 2 == 0) {
		//printf("A second has passed.\n");
		process_switch();
	}
}

void timer_init() {
	u32 divisor = PIT_CLOCK_FREQUENCY_HZ / TIMER_DESIRED_FREQUENCY_HZ;
	io_byte_out(PIT_COMMAND_PORT, 0x36);
	io_byte_out(PIT_DATA_PORT_0, (u8)(divisor & 0xFF));
	io_byte_out(PIT_DATA_PORT_0, (u8)((divisor >> 8) & 0xFF));
	interrupt_register_handler(timer_interrupt_handler, IRQ0);
}
#include "interrupt.h"
#include "asm/io.h"
#include "util/printf.h"
#include "asm/interrupt.h"
#include "timer.h"
#include "util/printf.h"

#define PIC1 0x20                   // IO port for master PIC
#define PIC2 0xA0                   // IO port for slave PIC
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2+1)
#define PIC_EOI	0x20		        // End-of-interrupt command code

#define ICW1_ICW4 0x01              // ICW4 (not) needed
#define ICW1_SINGLE	0x02            // Single (cascade) mode
#define ICW1_INTERVAL4 0x04         // Call address interval 4 (8)
#define ICW1_LEVEL 0x08             // Level triggered (edge) mode
#define ICW1_INIT 0x10              // Initialization - required!
 
#define ICW4_8086 0x01              // 8086/88 (MCS-80/85) mode
#define ICW4_AUTO 0x02              // Auto (normal) EOI
#define ICW4_BUF_SLAVE 0x08         // Buffered mode/slave
#define ICW4_BUF_MASTER	0x0C        // Buffered mode/master
#define ICW4_SFNM 0x10              // Special fully nested (not)

#define PIC1_VECTOR_OFFSET 0x20     // The remapped offset of the master PIC
#define PIC2_VECTOR_OFFSET 0x28     // The remapped offset of the slave PIC

// For interrupt gates, x86 expects:
// 1 bit -> segment present flag (1)
// 2 bits -> descriptor privilege level (00)
// 5 bits -> 0D110. D specifies the size of the gate (1 = 32 bits, 0 = 16 bits). The others are mandatory to specify an Interrupt Gate
#define IDT_INTERRUPT_GATE_FLAGS 0b10001110

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

static Interrupt_Handler interrupt_handlers[IDT_SIZE];

// Fill the IDT_Descriptor structure, following the x86 protocol
static void idt_set_descriptor(IDT_Descriptor* idt_descriptor, u32 base, u16 selector, u8 flags) {
	idt_descriptor->offset_high = (u16)((u32)base >> 16);
	idt_descriptor->offset_low = (u16)((u32)base & 0xFFFF);
	idt_descriptor->always0 = 0;
	idt_descriptor->flags = IDT_INTERRUPT_GATE_FLAGS | 0x60;
	idt_descriptor->selector = 0x08;
}

// The PIC (Programmable Interrupt Controller) is a chip present in the x86 architecture that manages the hardware interrupts.
// This chip is connected to all interrupt-driven devices via multiple buses and connected to the processor in a single bus.
// It basically acts as a multiplexer, managing the interrupts that are received, ordering them by priority, and sending one at a time.
// Since IBM PC/AT 8259, the PC architecture has 2 PIC chips: the master and the slave. The slave is connected to the master as if it was a hardware device.
// The vector offset configured in each PIC determines its interrupt number. The problem is that the master PIC is mapped to 0x08, meaning that interrupts 0x08 to 0x0F
// are assigned to it. But these interruptions are already used by exceptions when the CPU is running in protected mode.
// This function remaps the vector offset of the master PIC to 0x20 (meaning interruption numbers will go from 0x20 to 0x28)
// And the vector offset of the slave PIC to 0x28 (meaning interruption numbers will go from 0x28 to 0x2F)
// Note that there are 32 reserved exceptions in x86 protected mode, meaning that 0x20 is exactly when they finish in the IDT
static void pic_remap() {
	// The only way to change the vector offsets used by the 8259 PIC is to re-initialize it.
	// In order to re-initialize the PIC, we send the ICW1_INIT command. This command makes the PIC wait for 3 extra "initialisation words" (ICW2, ICW3, ICW4) on the data port.
	io_byte_out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);   // starts the initialization sequence (in cascade mode)
	io_byte_out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_byte_out(PIC1_DATA, PIC1_VECTOR_OFFSET);         // ICW2: Master PIC vector offset
	io_byte_out(PIC2_DATA, PIC2_VECTOR_OFFSET);         // ICW2: Slave PIC vector offset
	io_byte_out(PIC1_DATA, 0x4);                        // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	io_byte_out(PIC2_DATA, 0x2);                        // ICW3: tell Slave PIC its cascade identity (0000 0010)
	io_byte_out(PIC1_DATA, ICW4_8086);
	io_byte_out(PIC2_DATA, ICW4_8086);
	io_byte_out(PIC1_DATA, 0x0);                        // Here we set the masks... Set to 0x0 for now.
	io_byte_out(PIC2_DATA, 0x0);                        // Here we set the masks... Set to 0x0 for now.
}

// Creates the Interruption Descriptor Table, needed in x86
// Also loads the table in the IDTR register and enables interrupts
void interrupt_init() {
	pic_remap();

	// We directly write the IDT to IDT_BASE address
	IDT* idt = (IDT*)IDT_BASE;

	// Set all IDT descriptors
	// Each IDT descriptor contain the pointer to its corresponding interrupt-handler routine
	// @TODO: 0x08 specifies the segment selector to be used, 0x08 points to CODE_SEG in the current GDT.
	// I think we need to define the GDT in C or create the GDT consts somewhere.
	idt_set_descriptor(&idt->descriptors[0], (u32)interrupt_isr0, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[1], (u32)interrupt_isr1, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[2], (u32)interrupt_isr2, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[3], (u32)interrupt_isr3, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[4], (u32)interrupt_isr4, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[5], (u32)interrupt_isr5, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[6], (u32)interrupt_isr6, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[7], (u32)interrupt_isr7, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[8], (u32)interrupt_isr8, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[8], (u32)interrupt_isr8, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[10], (u32)interrupt_isr10, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[11], (u32)interrupt_isr11, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[12], (u32)interrupt_isr12, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[13], (u32)interrupt_isr13, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[14], (u32)interrupt_isr14, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[15], (u32)interrupt_isr15, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[16], (u32)interrupt_isr16, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[17], (u32)interrupt_isr17, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[18], (u32)interrupt_isr18, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[18], (u32)interrupt_isr18, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[20], (u32)interrupt_isr20, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[21], (u32)interrupt_isr21, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[22], (u32)interrupt_isr22, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[23], (u32)interrupt_isr23, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[24], (u32)interrupt_isr24, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[25], (u32)interrupt_isr25, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[26], (u32)interrupt_isr26, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[27], (u32)interrupt_isr27, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[28], (u32)interrupt_isr28, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[28], (u32)interrupt_isr28, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[30], (u32)interrupt_isr30, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[31], (u32)interrupt_isr31, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[32], (u32)interrupt_irq0, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[33], (u32)interrupt_irq1, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[34], (u32)interrupt_irq2, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[35], (u32)interrupt_irq3, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[36], (u32)interrupt_irq4, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[37], (u32)interrupt_irq5, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[38], (u32)interrupt_irq6, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[39], (u32)interrupt_irq7, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[40], (u32)interrupt_irq8, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[41], (u32)interrupt_irq9, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[42], (u32)interrupt_irq10, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[43], (u32)interrupt_irq11, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[44], (u32)interrupt_irq12, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[45], (u32)interrupt_irq13, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[46], (u32)interrupt_irq14, 0x08, IDT_INTERRUPT_GATE_FLAGS);

	// The total size of the descriptors
	u32 idt_descriptors_len = sizeof(IDT_Descriptor) * IDT_SIZE;

	// The base and the limit are being stored after the descriptors in memory.
	// They are loaded in the IDTR register. The processor expects 16 bytes for the limit and 32 bytes for the base address.
	idt->base = IDT_BASE;
	idt->limit = idt_descriptors_len - 1; // -1 is necessary. processor wants the address of the last descriptor.

	// Flushes the IDT, meaning filling the IDTR register
	interrupt_idt_flush(IDT_BASE + idt_descriptors_len);
	// Enables interrupts
	interrupt_enable();
}

void interrupt_register_handler(Interrupt_Handler interrupt_handler, u32 interrupt_number) {
	interrupt_handlers[interrupt_number] = interrupt_handler;
}

void isr_handler(Interrupt_Handler_Args args) {
	Interrupt_Handler interrupt_handler = interrupt_handlers[args.int_no];

	if (interrupt_handler) {
		interrupt_handler(&args);
	} else {
		printf("ISR Handler called for unhandled INT number: %u\n", args.int_no);
	}
}

void irq_handler(Interrupt_Handler_Args args) {
	// Send an EOI (end of interrupt) signal to the PICs.

	// If this interrupt involved the slave.
	if (args.int_no >= PIC2_VECTOR_OFFSET) {
		// Notify slave
		io_byte_out(PIC2, PIC_EOI);
	}
	// Notify master
	io_byte_out(PIC1, PIC_EOI);

	Interrupt_Handler interrupt_handler = interrupt_handlers[args.int_no];

	if (interrupt_handler) {
		interrupt_handler(&args);
	} else {
		printf("IRQ Handler called for unhandled INT number: %u\n", args.int_no);
	}
}
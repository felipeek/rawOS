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
	// ISR 0-31 are reserved to software exceptions triggered by the processor
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
	// IRQ 0-15 are reserved to hardware interrupts.
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
	// ISR 47-255 are reserved to software interrupts.
	idt_set_descriptor(&idt->descriptors[47], (u32)interrupt_isr47, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[48], (u32)interrupt_isr48, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[49], (u32)interrupt_isr49, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[50], (u32)interrupt_isr50, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[51], (u32)interrupt_isr51, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[52], (u32)interrupt_isr52, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[53], (u32)interrupt_isr53, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[54], (u32)interrupt_isr54, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[55], (u32)interrupt_isr55, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[56], (u32)interrupt_isr56, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[57], (u32)interrupt_isr57, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[58], (u32)interrupt_isr58, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[59], (u32)interrupt_isr59, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[60], (u32)interrupt_isr60, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[61], (u32)interrupt_isr61, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[62], (u32)interrupt_isr62, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[63], (u32)interrupt_isr63, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[64], (u32)interrupt_isr64, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[65], (u32)interrupt_isr65, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[66], (u32)interrupt_isr66, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[67], (u32)interrupt_isr67, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[68], (u32)interrupt_isr68, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[69], (u32)interrupt_isr69, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[70], (u32)interrupt_isr70, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[71], (u32)interrupt_isr71, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[72], (u32)interrupt_isr72, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[73], (u32)interrupt_isr73, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[74], (u32)interrupt_isr74, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[75], (u32)interrupt_isr75, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[76], (u32)interrupt_isr76, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[77], (u32)interrupt_isr77, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[78], (u32)interrupt_isr78, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[79], (u32)interrupt_isr79, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[80], (u32)interrupt_isr80, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[81], (u32)interrupt_isr81, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[82], (u32)interrupt_isr82, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[83], (u32)interrupt_isr83, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[84], (u32)interrupt_isr84, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[85], (u32)interrupt_isr85, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[86], (u32)interrupt_isr86, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[87], (u32)interrupt_isr87, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[88], (u32)interrupt_isr88, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[89], (u32)interrupt_isr89, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[90], (u32)interrupt_isr90, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[91], (u32)interrupt_isr91, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[92], (u32)interrupt_isr92, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[93], (u32)interrupt_isr93, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[94], (u32)interrupt_isr94, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[95], (u32)interrupt_isr95, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[96], (u32)interrupt_isr96, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[97], (u32)interrupt_isr97, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[98], (u32)interrupt_isr98, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[99], (u32)interrupt_isr99, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[100], (u32)interrupt_isr100, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[101], (u32)interrupt_isr101, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[102], (u32)interrupt_isr102, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[103], (u32)interrupt_isr103, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[104], (u32)interrupt_isr104, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[105], (u32)interrupt_isr105, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[106], (u32)interrupt_isr106, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[107], (u32)interrupt_isr107, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[108], (u32)interrupt_isr108, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[109], (u32)interrupt_isr109, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[110], (u32)interrupt_isr110, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[111], (u32)interrupt_isr111, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[112], (u32)interrupt_isr112, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[113], (u32)interrupt_isr113, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[114], (u32)interrupt_isr114, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[115], (u32)interrupt_isr115, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[116], (u32)interrupt_isr116, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[117], (u32)interrupt_isr117, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[118], (u32)interrupt_isr118, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[119], (u32)interrupt_isr119, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[120], (u32)interrupt_isr120, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[121], (u32)interrupt_isr121, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[122], (u32)interrupt_isr122, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[123], (u32)interrupt_isr123, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[124], (u32)interrupt_isr124, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[125], (u32)interrupt_isr125, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[126], (u32)interrupt_isr126, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[127], (u32)interrupt_isr127, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[128], (u32)interrupt_isr128, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[129], (u32)interrupt_isr129, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[130], (u32)interrupt_isr130, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[131], (u32)interrupt_isr131, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[132], (u32)interrupt_isr132, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[133], (u32)interrupt_isr133, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[134], (u32)interrupt_isr134, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[135], (u32)interrupt_isr135, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[136], (u32)interrupt_isr136, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[137], (u32)interrupt_isr137, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[138], (u32)interrupt_isr138, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[139], (u32)interrupt_isr139, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[140], (u32)interrupt_isr140, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[141], (u32)interrupt_isr141, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[142], (u32)interrupt_isr142, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[143], (u32)interrupt_isr143, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[144], (u32)interrupt_isr144, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[145], (u32)interrupt_isr145, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[146], (u32)interrupt_isr146, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[147], (u32)interrupt_isr147, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[148], (u32)interrupt_isr148, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[149], (u32)interrupt_isr149, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[150], (u32)interrupt_isr150, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[151], (u32)interrupt_isr151, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[152], (u32)interrupt_isr152, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[153], (u32)interrupt_isr153, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[154], (u32)interrupt_isr154, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[155], (u32)interrupt_isr155, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[156], (u32)interrupt_isr156, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[157], (u32)interrupt_isr157, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[158], (u32)interrupt_isr158, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[159], (u32)interrupt_isr159, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[160], (u32)interrupt_isr160, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[161], (u32)interrupt_isr161, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[162], (u32)interrupt_isr162, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[163], (u32)interrupt_isr163, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[164], (u32)interrupt_isr164, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[165], (u32)interrupt_isr165, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[166], (u32)interrupt_isr166, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[167], (u32)interrupt_isr167, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[168], (u32)interrupt_isr168, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[169], (u32)interrupt_isr169, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[170], (u32)interrupt_isr170, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[171], (u32)interrupt_isr171, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[172], (u32)interrupt_isr172, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[173], (u32)interrupt_isr173, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[174], (u32)interrupt_isr174, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[175], (u32)interrupt_isr175, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[176], (u32)interrupt_isr176, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[177], (u32)interrupt_isr177, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[178], (u32)interrupt_isr178, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[179], (u32)interrupt_isr179, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[180], (u32)interrupt_isr180, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[181], (u32)interrupt_isr181, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[182], (u32)interrupt_isr182, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[183], (u32)interrupt_isr183, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[184], (u32)interrupt_isr184, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[185], (u32)interrupt_isr185, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[186], (u32)interrupt_isr186, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[187], (u32)interrupt_isr187, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[188], (u32)interrupt_isr188, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[189], (u32)interrupt_isr189, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[190], (u32)interrupt_isr190, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[191], (u32)interrupt_isr191, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[192], (u32)interrupt_isr192, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[193], (u32)interrupt_isr193, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[194], (u32)interrupt_isr194, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[195], (u32)interrupt_isr195, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[196], (u32)interrupt_isr196, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[197], (u32)interrupt_isr197, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[198], (u32)interrupt_isr198, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[199], (u32)interrupt_isr199, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[200], (u32)interrupt_isr200, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[201], (u32)interrupt_isr201, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[202], (u32)interrupt_isr202, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[203], (u32)interrupt_isr203, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[204], (u32)interrupt_isr204, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[205], (u32)interrupt_isr205, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[206], (u32)interrupt_isr206, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[207], (u32)interrupt_isr207, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[208], (u32)interrupt_isr208, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[209], (u32)interrupt_isr209, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[210], (u32)interrupt_isr210, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[211], (u32)interrupt_isr211, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[212], (u32)interrupt_isr212, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[213], (u32)interrupt_isr213, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[214], (u32)interrupt_isr214, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[215], (u32)interrupt_isr215, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[216], (u32)interrupt_isr216, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[217], (u32)interrupt_isr217, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[218], (u32)interrupt_isr218, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[219], (u32)interrupt_isr219, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[220], (u32)interrupt_isr220, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[221], (u32)interrupt_isr221, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[222], (u32)interrupt_isr222, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[223], (u32)interrupt_isr223, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[224], (u32)interrupt_isr224, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[225], (u32)interrupt_isr225, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[226], (u32)interrupt_isr226, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[227], (u32)interrupt_isr227, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[228], (u32)interrupt_isr228, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[229], (u32)interrupt_isr229, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[230], (u32)interrupt_isr230, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[231], (u32)interrupt_isr231, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[232], (u32)interrupt_isr232, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[233], (u32)interrupt_isr233, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[234], (u32)interrupt_isr234, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[235], (u32)interrupt_isr235, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[236], (u32)interrupt_isr236, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[237], (u32)interrupt_isr237, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[238], (u32)interrupt_isr238, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[239], (u32)interrupt_isr239, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[240], (u32)interrupt_isr240, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[241], (u32)interrupt_isr241, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[242], (u32)interrupt_isr242, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[243], (u32)interrupt_isr243, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[244], (u32)interrupt_isr244, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[245], (u32)interrupt_isr245, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[246], (u32)interrupt_isr246, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[247], (u32)interrupt_isr247, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[248], (u32)interrupt_isr248, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[249], (u32)interrupt_isr249, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[250], (u32)interrupt_isr250, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[251], (u32)interrupt_isr251, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[252], (u32)interrupt_isr252, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[253], (u32)interrupt_isr253, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[254], (u32)interrupt_isr254, 0x08, IDT_INTERRUPT_GATE_FLAGS);
	idt_set_descriptor(&idt->descriptors[255], (u32)interrupt_isr255, 0x08, IDT_INTERRUPT_GATE_FLAGS);

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
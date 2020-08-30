#include "interrupt.h"
#include "asm/io.h"
#include "screen.h"
#include "asm/interrupt.h"
#include "util.h"

// For interrupt gates, x86 expects:
// 1 bit -> segment present flag (1)
// 2 bits -> descriptor privilege level (00)
// 5 bits -> 0D110. D specifies the size of the gate (1 = 32 bits, 0 = 16 bits). The others are mandatory to specify an Interrupt Gate
#define IDT_INTERRUPT_GATE_FLAGS 0b10001110

// Fill the IDT_Descriptor structure, following the x86 protocol
static void idt_set_descriptor(IDT_Descriptor* idt_descriptor, u32 base, u16 selector, u8 flags) {
    idt_descriptor->offset_high = (u16)((u32)base >> 16);
    idt_descriptor->offset_low = (u16)((u32)base & 0xFFFF);
    idt_descriptor->always0 = 0;
    idt_descriptor->flags = IDT_INTERRUPT_GATE_FLAGS;
    idt_descriptor->selector = 0x08;
}

// Creates the Interruption Descriptor Table, needed in x86
// Also loads the table in the IDTR register and enables interrupts
void idt_init() {
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

typedef struct {
   u32 edi, esi, ebp, esp, ebx, edx, ecx, eax;      // Pushed by pusha.
   u32 int_no, err_code;                            // Interrupt number and error code (if applicable)
   u32 eip, cs, eflags, useresp, ss;                // Pushed by the processor automatically.
} ISR_Handler_Args; 

extern Screen s;
void isr_handler(ISR_Handler_Args args) {
    screen_print_ptr(&s, &s);
}
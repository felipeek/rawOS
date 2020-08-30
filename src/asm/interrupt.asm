extern isr_handler
global interrupt_idt_flush
global interrupt_enable
global interrupt_disable

; Declare a common ISR for interruptions that DO NOT provide an error code
; The interruption number is passed as a stack parameter
; Note that the processor just calls the ISR without identifying the interruption,
; so we do need separate functions for each one.
%macro ISR_NOERRCODE 1
    global interrupt_isr%1
    interrupt_isr%1:
        cli
        push byte 0
        push byte %1
        jmp isr_common_stub
%endmacro

; Declare a common ISR for interruptions that PROVIDE an error code
; The interruption number is passed as a stack parameter
; Note that the processor just calls the ISR without identifying the interruption,
; so we do need separate functions for each one.
%macro ISR_ERRCODE 1
    global interrupt_isr%1
    interrupt_isr%1:
        cli
        push byte %1
        jmp isr_common_stub
%endmacro

; This code block is called by all ISRs. 
isr_common_stub:
    pusha               ; Push all the registers to the stack, so the isr_handler can have access to them.
    call isr_handler    ; Call the high-level handler to handle the interruption
    popa                ; Clear the stack.
    add esp, 8          ; ?
    sti
    iret

; Create all needed ISRs, using the macros defined above
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2 
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

interrupt_idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

interrupt_disable:
    cli
    ret

interrupt_enable:
    sti
    ret
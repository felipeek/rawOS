global io_byte_in
global io_byte_out

section .data
section .text

; the following registers must be preserved by the callee in cdecl (x86): EBX, ESI, EDI, EBP... EAX, EDX and ECX are call-clobbered registers

; unsigned char io_byte_in(u16 port)
io_byte_in:
    push ebp
    mov ebp, esp

    mov dx, [ebp + 8]           ; store the desired port in dx
    in al, dx                   ; store the register value in al

    pop ebp
    ret

; void io_byte_out(u16 port, u8 data)
io_byte_out:
    push ebp
    mov ebp, esp

    mov dx, [ebp + 8]           ; store the desired port in dx
    mov al, [ebp + 12]          ; store the data to be sent in al
    out dx, al

    pop ebp
    ret
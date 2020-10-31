global gdt_flush
global gdt_tss_flush

; void gdt_configure(u32 gdt_ptr)
gdt_flush:
	mov eax, [esp + 4]						; GDT Pointer

	cli
	lgdt [eax]
	jmp 0x08:gdt_configure_jmp        		; perform far-jump to clean the CPU pipeline
gdt_configure_jmp:
	mov ax, 0x10     		       			; set all segment registers to GDT Data Segment
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	;sti									; for some reason we dont need to re-enable interrupts.
											; im not sure, maybe lgdt also re-enable interupts?
	ret

gdt_tss_flush:
   mov ax, 0x2B      ; Load the index of our TSS structure - The index is
                     ; 0x28, as it is the 5th selector and each is 8 bytes
                     ; long, but we set the bottom two bits (making 0x2B)
                     ; so that it has an RPL of 3, not zero.
   ltr ax            ; Load 0x2B into the task state register.
   ret
global gdt_configure

; void gdt_configure(u32 gdt_ptr)
gdt_configure:
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
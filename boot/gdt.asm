gdt_start:

gdt_null:
	; The first entry of the GDT must be the null descriptor, so we have 8 bytes set to 0x0
	dd 0x00000000
	dd 0x00000000

gdt_code:
	; We start with 2 bytes for Segment Limit[15:00]. Should be 0xFFFF
	dw 0xFFFF
	; Next we have 2 bytes for Base[15:00]. Should be 0x0000.
	dw 0x0000
	; Next we have 1 byte for Base[23:16]. Should be 0x00.
	db 0x00
	; Next we have 1 bit for segment present, 2 bits for descriptor privilege level and 1 bit for descriptor type.
	; Segment Present: 1, because the segment is present in memory (used for virtual memory)
	; Descriptor Privilege Level: 00: ring 0 - highest privilege.
	; Desciptor Type: (0 = system, 1 = code or data): 1, because this is the code segment.
	; Next we have 4 bits to define the segment type, which expands in:
	; 1 bit for 'code'. Should be 1 for code and 0 for data. 1 in this case. The meaning of the next 3 fields are different depending on if this is set to code or data.
	; 1 bit for 'conforming'. Not conforming means code in a segment with lower privilege may not call code in this segment. Set to 0.
	; 1 bit for 'readable'. 1 if readable, 0 if execute-only. Set to 1.
	; 1 bit for 'accessed'. CPU sets this when segment is accessed. Set to 0.
	db 10011010b
	; Next, we have 4 bits for granularity, default operation size, 64-bit code segment and AVL.
	; Granularity: 1, so our limit is multiplied by the page size (4K)
	; Default operation size: 1, because we want 32 bits.
	; 64-bit Code Segment: 0 for now, stick with 32 bits.
	; AVL: Available for use by system software. Not using for now, so 0.
	; The next 4 bits are the 4 most significant bits of the segment limit (Seg. Limit[19:16]). Our limit is 0xFFFFF
	db 11001111b
	; End with Base[31:24]. The base is 0x0
	db 0x00

gdt_data:
	; We start with 2 bytes for Segment Limit[15:00]. Should be 0xFFFF
	dw 0xFFFF
	; Next we have 2 bytes for Base[15:00]. Should be 0x0000.
	dw 0x0000
	; Next we have 1 byte for Base[23:16]. Should be 0x00.
	db 0x00
	; Next we have 1 bit for segment present, 2 bits for descriptor privilege level and 1 bit for descriptor type.
	; Segment Present: 1, because the segment is present in memory (used for virtual memory)
	; Descriptor Privilege Level: 00: ring 0 - highest privilege.
	; Desciptor Type: (0 = system, 1 = code or data): 1, because this is the code segment.
	; Next we have 4 bits to define the segment type, which expands in:
	; 1 bit for 'code'. Should be 1 for code and 0 for data. 0 in this case. The meaning of the next 3 fields are different depending on if this is set to code or data.
	; 1 bit for 'expand down'. Set to 0.
	; 1 bit for 'writable'. 1 if writable, 0 if read-only. Set to 1.
	; 1 bit for 'accessed'. CPU sets this when segment is accessed. Set to 0.
	db 10010010b
	; Next, we have 4 bits for granularity, default operation size, 64-bit code segment and AVL.
	; Granularity: 1, so our limit is multiplied by the page size (4K)
	; Default operation size: 1, because we want 32 bits.
	; 64-bit Code Segment: 0 for now, stick with 32 bits.
	; AVL: Available for use by system software. Not using for now, so 0.
	; The next 4 bits are the 4 most significant bits of the segment limit (Seg. Limit[19:16]). Our limit is 0xFFFFF
	db 11001111b
	; End with Base[31:24]. The base is 0x0
	db 0x00

gdt_end:

gdt_descriptor:
	; The GDT descriptor describes the GDT. It starts with 16 bits to define the GDT size and ends with 32 bits specifying the GDT address.
	dw gdt_end - gdt_start - 1
	dd gdt_start

; When running in 32-bit mode, the segment registers contain references to GDT/LDT. When accessing the GDT, they need to reference one of the
; GDT entries. In order to do that, they need to specify the address of the entry inside the GDT, relative to GDT's start address.
; Below we create some handy constants so we can use later.
GDT_CODE_SEG equ gdt_code - gdt_start
GDT_DATA_SEG equ gdt_data - gdt_start
[org 0x7c00]

mov [BOOT_DRIVE], dl            ; BIOS stores the boot drive in dl

mov bp, 0x8000
mov sp, bp

cli                             ; disables interrupts so we can go to protected mode
lgdt [gdt_descriptor]           ; load the basic flat model GDT
mov eax, cr0                    ; mov cr0 to eax
or eax, 0x1                     ; set flag to switch to protected mode
mov cr0, eax                    ; set cr0, effectively switching to 32-bit mode
jmp GDT_CODE_SEG:init_pm        ; perform far-jump to clean the CPU pipeline

[bits 32]
init_pm:
mov ax, GDT_DATA_SEG            ; set all segment registers to GDT_DATA_SEG
mov ds, ax
mov ss, ax
mov es, ax
mov fs, ax
mov gs, ax

mov ebp, 0x90000                ; now that we can address 32 bits, we update the stack position to be
mov esp, ebp                    ; just under the extended BIOS data area

push switch_to_pm_msg
call util_print_string

jmp $

BOOT_DRIVE: db 0

hex_data:
db 0xAB, 0xCD, 0x35, 0xF8
hello_world_string:
db 'Hello World!', 0
hello_world_string2:
db 'Hello My Beautiful World!', 0
switch_to_pm_msg:
db 'Switched to protected mode!', 0

%include "src/util_16bits.asm"
%include "src/util.asm"
%include "src/gdt.asm"

times 510-($-$$) db 0

dw 0xaa55

times 256 dw 0xDADA
times 256 dw 0xF1F2
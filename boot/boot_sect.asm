[org 0x7c00]

mov [BOOT_DRIVE], dl            ; BIOS stores the boot drive in dl

mov bp, 0x8000                  ; Set stack to start at 0x8000
mov sp, bp

push LOAD_KERNEL_MSG
call util_16bits_print_string
add esp, 2

call load_kernel                ; loads the kernel to KERNEL_OFFSET

; Switch to protect mode...

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

	push SWITCH_TO_PM_MSG
	call util_print_string
	add esp, 4

	call KERNEL_OFFSET

	jmp $

KERNEL_OFFSET equ 0x1000
BOOT_DRIVE: db 0

SWITCH_TO_PM_MSG: db 'Switched to protected mode!', 0
LOAD_KERNEL_MSG: db 'Loading kernel...', 0

[bits 16]

load_kernel:
	push bp
	mov bp, sp

	push ax

	mov ah, 24                      ; Number of sectors to read
									; for now load all sectors that we can, for dev purposes
	mov al, [BOOT_DRIVE]            ; Read from boot drive
	push ax
	mov ah, 0                       ; Track/cylinder number
	mov al, 2                       ; Start at sector 2
	push ax
	push 0x0                        ; Head number + padding
	push KERNEL_OFFSET              ; Read sectors in this location
	call util_16bits_read_disk_sectors
	add esp, 8                      ; clear stack (cdecl-like)

	pop ax
	pop bp
	ret

%include "boot/util_16bits.asm"
%include "boot/util.asm"
%include "boot/gdt.asm"

times 510-($-$$) db 0

dw 0xaa55
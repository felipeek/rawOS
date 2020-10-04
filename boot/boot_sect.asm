[org 0x7c00]

mov [BOOT_DRIVE], dl            ; BIOS stores the boot drive in dl

mov bp, 0x8000                  ; Set stack to start at 0x8000
mov sp, bp

push LOAD_KERNEL_MSG
call util_16bits_print_string
add esp, 2

;mov ax, 0x1000
;mov ds, ax
;mov bx, [0x0000]
;mov ax, 0x0
;mov ds, ax
;mov [0xAAAA], bx
;push 0xAAAA
;push 2
;call util_16bits_print_hex
;add esp, 4

call load_kernel                ; loads the kernel to KERNEL_OFFSET

;mov ax, 0x1000
;mov ds, ax
;mov bx, [0x0000]
;mov ax, 0x0
;mov ds, ax
;mov [0xAAAA], bx
;push 0xAAAA
;push 2
;call util_16bits_print_hex
;add esp, 4

;jmp $

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

	call KERNEL_OFFSET_PROTECTED_MODE

	jmp $

; NOTE about the kernel address: for now, we are leveraging the real mode segment registers to load the kernel to address 0x10000.
; This is done because if we load to, let's say, 0x1000, we may overwrite BIOS code, which is loaded at 0x7c00 (see first line)
; The kernel is already large enough to overwrite the BIOS, which will lead to major issues during the bootstrap process.
; Later on, we might want to implement our own bootstrapper, and if it is small enough we can load it to 0x1000 (or any addr inside the 16-bit
; addres space)
; Since now we are loading to 0x10000, the kernel code can grow until 0xFFFFF safely, and nothing below 0x10000 will be touched.
; If, however, the kernel size reaches 0xFFFFF, then we will need to write the bootstrapper, since we will not be able to access higher
; address in real mode.
; Address that we will jump to when starting the kernel.
KERNEL_OFFSET_PROTECTED_MODE equ 0x10000
; Address that we will load the kernel
; The address is calculated as: (16 * KERNEL_OFFSET_REAL_MODE_SEGMENT + KERNEL_OFFSET_REAL_MODE)
KERNEL_OFFSET_REAL_MODE_SEGMENT equ 0x1000
KERNEL_OFFSET_REAL_MODE equ 0x0000
; NOTE: These address MUST be the same! (the protected mode addr should be the same as the other after the evaluation)
BOOT_DRIVE: db 0

SWITCH_TO_PM_MSG: db 'Switched to protected mode!', 0
LOAD_KERNEL_MSG: db 'Loading kernel...', 0

[bits 16]

load_kernel:
	push bp
	mov bp, sp

	push ax

	; Move the kernel segment offset to the es segment register.
	; BIOS will use this when loading the sectors.
	mov ax, KERNEL_OFFSET_REAL_MODE_SEGMENT
	mov es, ax

	mov ah, 48                      ; Number of sectors to read
									; for now load all sectors that we can, for dev purposes
	mov al, [BOOT_DRIVE]            ; Read from boot drive
	push ax
	mov ah, 0                       ; Track/cylinder number
	mov al, 2                       ; Start at sector 2
	push ax
	push 0x0                        ; Head number + padding
	push KERNEL_OFFSET_REAL_MODE    ; Read sectors in this location (+ 16 * SEGMENT)
	call util_16bits_read_disk_sectors
	add esp, 8                      ; clear stack (cdecl-like)

	mov ax, 0x0000
	mov es, ax

	pop ax
	pop bp
	ret

%include "boot/util_16bits.asm"
%include "boot/util.asm"
%include "boot/gdt.asm"

times 510-($-$$) db 0

dw 0xaa55
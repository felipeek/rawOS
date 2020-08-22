[org 0x7c00]

mov [BOOT_DRIVE], dl            ; BIOS stores the boot drive in dl

mov bp, 0x8000
mov sp, bp

;push hex_data
;push 4
;call util_16bits_print_hex

mov ah, 0x02            ; read 2 sectors
mov al, [BOOT_DRIVE]
push ax
mov ah, 0x0
mov al, 0x2
push ax
mov ah, 0x0
push ax
push 0x9000
call util_16bits_read_disk_sectors

push 0x9000
push 1024
call util_16bits_print_hex

;push hello_world_string
;call util_16bits_print_string
;add esp, 0x2
;push hello_world_string2
;call util_16bits_print_string
;add esp, 0x2

jmp $

BOOT_DRIVE: db 0

hex_data:
db 0xAB, 0xCD, 0x35, 0xF8
hello_world_string:
db 'Hello World!', 0
hello_world_string2:
db 'Hello My Beautiful World!', 0

%include "src/util_16bits.asm"

times 510-($-$$) db 0

dw 0xaa55

times 256 dw 0xDADA
times 256 dw 0xF1F2
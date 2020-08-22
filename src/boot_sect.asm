[org 0x7c00]

mov bp, 0x8000
mov sp, bp

push hex_data
push 4
call util_16bits_print_hex

;push hello_world_string
;call util_16bits_print_string
;add esp, 0x2
;push hello_world_string2
;call util_16bits_print_string
;add esp, 0x2

jmp $

hex_data:
db 0xAB, 0xCD, 0x35, 0xF8
hello_world_string:
db 'Hello World!', 0
hello_world_string2:
db 'Hello My Beautiful World!', 0

%include "src/util_16bits.asm"

times 510-($-$$) db 0

dw 0xaa55

; Print N bytes in hex.
; Arguments (Push order):
; Pointer to content that should be printed (16 bits)
; Number of bytes to be printed (16 bits)
util_16bits_print_hex:
push bp
mov bp, sp

push ax
push bx
push cx
push dx

mov cx, [bp + 0x4]      ; now cx has the number of bytes to be printed
mov bx, [bp + 0x6]      ; now bx has the pointer to the content
mov ah, 0x0e            ; to write text in teletype mode, BIOS expects ah=0x0e and INT 0x10

; print 0x
mov al, '0'
int 0x10
mov al, 'x'
int 0x10

util_16bits_print_hex_loop:

cmp cx, 0
je util_16bits_print_hex_out_loop

mov dl, [bx]

mov al, dl
shr al, 4

cmp al, 0xA
jge util_16bits_print_hex_letter

add al, '0'
jmp util_16bits_print_hex_int

util_16bits_print_hex_letter:
sub al, 0xA
add al, 'A'

util_16bits_print_hex_int:
int 0x10

mov al, dl
and al, 0xF

cmp al, 0xA
jge util_16bits_print_hex_letter2

add al, '0'
jmp util_16bits_print_hex_int2

util_16bits_print_hex_letter2:
sub al, 0xA
add al, 'A'

util_16bits_print_hex_int2:
int 0x10

dec cx
inc bx
jmp util_16bits_print_hex_loop

util_16bits_print_hex_out_loop:

pop dx
pop cx
pop bx
pop ax

pop bp
ret

; Print nul-terminated string to screen, using BIOS
; Arguments (Push order):
; Pointer to string (16 bits)
util_16bits_print_string:
push bp
mov bp, sp

push ax
push bx

mov bx, [bp + 0x4]      ; now bx has the pointer to the nul-terminated string
mov ah, 0x0e            ; to write text in teletype mode, BIOS expects ah=0x0e and INT 0x10

util_16bits_print_string_loop:

mov al, [bx]

cmp al, 0               ; if byte is \0, we are done
je util_16bits_print_string_out_loop

int 0x10                ; print byte using BIOS
inc bx

jmp util_16bits_print_string_loop

util_16bits_print_string_out_loop:

pop bx
pop ax

pop bp
ret
[bits 16]

; Read N sectors from disk, using BIOS
; Arguments (Push order):
; Number of sectors to read (8 bits) [H]
; Drive number (8 bits) [L]
; Track/cylinder number (8 bits) [H]
; Sector number (8 bits) [L]
; Head number (8 bits) [H]
; Padding (8 bits) [L]
; Buffer address (16 bits) [Note: caller needs to set ES register]
util_16bits_read_disk_sectors:
push bp
mov bp, sp

push ax
push bx
push cx
push dx

mov bx, [bp + 0x4]      ; now bx has the buffer address
mov ax, [bp + 0x6]      ; now ah has the head number
mov cx, [bp + 0x8]      ; now ch has the track/cylinder number and cl has the sector number
mov dx, [bp + 0xA]      ; now dh has the # of sectors and dl has the drive number

mov al, dh              ; BIOS expects # of sectors in al
mov dh, ah              ; BIOS expects head number in dh
mov ah, 0x02            ; BIOS expects ah=0x2 and INT 0x13 to read disk sectors
; BIOS expects track/cylinder number in ch and sector number in cl. They are already correct
; BIOS expects drive number in dl. It is already correct

int 0x13                ; call BIOS to read the sector

jc util_16bits_read_disk_sectors_error

mov dx, [bp + 0xA]      ; restore number # sectors in dh
cmp dh, al
jne util_16bits_read_disk_sectors_error

pop dx
pop cx
pop bx
pop ax

pop bp
ret

util_16bits_read_disk_sectors_error:
push DISK_ERROR_MSG
call util_16bits_print_string
jmp $

DISK_ERROR_MSG db "Error reading from disk", 0

; Print N bytes in hex, using BIOS
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

mov dl, [bx]            ; move next byte to be printed to dl

mov al, dl              ; move dl to al
shr al, 4               ; shift right 4 bits, so we can print the most significant nibble first.

cmp al, 0xA
jge util_16bits_print_hex_letter    ; if we are dealing with A, B, C, D, E or F, we need to treat them differently, because of ASCII...

add al, '0'             ; in the normal case, we add '0' to the number
jmp util_16bits_print_hex_int

util_16bits_print_hex_letter:
sub al, 0xA             ; in the special ABCDEF case, we first subtract 0xA, so A will become 0, B will become 1, and so on...
add al, 'A'             ; now we just add 'A'

util_16bits_print_hex_int:
int 0x10                ; print using BIOS

mov al, dl              ; move dl to al
and al, 0xF             ; now we print the less significant nibble

cmp al, 0xA
jge util_16bits_print_hex_letter2

add al, '0'
jmp util_16bits_print_hex_int2

util_16bits_print_hex_letter2:
sub al, 0xA
add al, 'A'

util_16bits_print_hex_int2:
int 0x10

dec cx                  ; decrement the number of bytes that need to be printed
inc bx                  ; increment content's address
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
[org 0x7c00]

mov bp, 0x8000
mov sp, bp

push 'B'            ; push B to the stack

push 'A'            ; push argument 'A' to stack
push 26             ; push argument 26 to stack
call print_alphabet ; call func
add sp, 0x4         ; clear the stack, following _cdecl principles. (caller is doing it instead of callee)

mov ax, [bp - 0x2]  ; if everything went fine, here we expect B
mov ah, 0x0e
int 0x10            ; print B

pop ax              ; if everything went fine, here we also expect B
mov ah, 0x0e
int 0x10            ; print B

jmp $

print_alphabet:
push bp             ; put bp in the stack
mov bp, sp          ; set bp <- sp
pusha               ; store all registers in the stack.. this include bp aswell.. kind of redundancy
sub sp, 32          ; allocate 32 bytes on the stack.

mov dl, 0           ; set dl <- 0
mov cl, [bp + 0x4]  ; set cl to one of the args. [bp + 0x4] because bp points to the stack of this function. bp + 0x2 points to the return address and bp + 0x4 points to the arg
mov dh, [bp + 0x6]  ; set cl to the other arg analogously

loop:
mov al, dh          ; al should receive the arg ('A')
add al, dl          ; add dl to al

mov bx, sp          ; move stack pointer to bx
add bx, 32          ; add 32 to bx, effectively going to the beggining of the 32-byte buffer that was previously allocated
sub bl, dl          ; make bl <- bl - dl, advancing dl bytes in the buffer
mov [bx], al        ; mov al to the buffer, in the correct position

inc dl              ; inc dl
cmp dl, cl          ; test whether dl is equal max number of letters
je out_loop         ; if it is, get out
jmp loop            ; if not, continue

out_loop:

mov dl, 0           ; set dl <- 0
mov ah, 0x0e        ; initialize ah with 0x0e, parameter to BIOS ISR

loop2:

mov bx, sp          ; move stack pointer to bx
add bx, 32          ; add 32 to bx, effectively going to the beggining of the 32-byte buffer that was previously allocated
sub bl, dl          ; make bl <- bl - dl, advancing dl bytes in the buffer
mov al, [bx]        ; move the content of the buffer in that position to al
int 0x10            ; call bios to write byte to screen

inc dl              ; inc dl
cmp dl, cl          ; test whether dl is equal max number of letters
je out_loop2        ; if it is, get out
jmp loop2           ; if not, continue

out_loop2:

add sp, 32          ; deallocate the 32 bytes in the stack.
popa                ; pop all registers, restoring their values
pop bp              ; pop the bp register, restoring its value
ret                 ; return back to caller

times 510-($-$$) db 0

dw 0xaa55

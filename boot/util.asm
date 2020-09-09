[bits 32]

VIDEO_MEMORY equ 0xB8000
WHITE_ON_BLACK equ 0x0f

; Print a string to the screen
; Arguments (Push order):
; Pointer to nul-terminated string (32 bits)
util_print_string:
	push ebp
	mov ebp, esp

	push eax
	push ebx
	push edx

	mov ebx, [ebp + 0x8]            ; ebx has a pointer to the buffer
	mov edx, VIDEO_MEMORY           ; edx has a pointer to the video memory

	util_print_string_loop:
		mov al, [ebx]                   ; al has the next byte to be printed

		cmp al, 0                       ; if al is \0 we stop
		je util_print_string_end

		mov ah, WHITE_ON_BLACK          ; ah has the char attributes
		mov [edx], ax                   ; move byte to be printed + attributes to video memory

		inc ebx                         ; step 1 byte in input buffer
		add edx, 2                      ; step 2 bytes in video memory
		jmp util_print_string_loop

	util_print_string_end:
		pop edx
		pop ebx
		pop eax

		pop ebp
		ret
global util_get_eip
global util_get_ebp
global util_get_esp

section .data
section .text

util_get_eip:
	pop eax
	jmp eax
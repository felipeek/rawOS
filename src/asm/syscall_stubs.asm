global syscall_print_stub
global syscall_print_stub_size

syscall_print_stub:
	mov eax, 0
	mov ebx, [esp + 4]
	int 0x80
	ret
syscall_print_stub_size: dd syscall_print_stub_size - syscall_print_stub
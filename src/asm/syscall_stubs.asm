global syscall_print_stub
global syscall_print_stub_size
global syscall_fork_stub
global syscall_fork_stub_size

; NOTE: syscall stubs are using stdcall for now

syscall_print_stub:
	mov eax, 0
	mov ebx, [esp + 4]
	int 0x80
	ret 4
syscall_print_stub_size: dd syscall_print_stub_size - syscall_print_stub

syscall_fork_stub:
	mov eax, 1
	int 0x80
	ret
syscall_fork_stub_size: dd syscall_fork_stub_size - syscall_fork_stub
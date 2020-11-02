global syscall_print_stub
global syscall_print_stub_size
global syscall_exit_stub
global syscall_exit_stub_size
global syscall_pos_cursor_stub
global syscall_pos_cursor_stub_size
global syscall_clear_screen_stub
global syscall_clear_screen_stub_size
global syscall_execve_stub
global syscall_execve_stub_size
global syscall_fork_stub
global syscall_fork_stub_size

; NOTE: syscall stubs are using stdcall for now

syscall_print_stub:
	mov eax, 0
	mov ebx, [esp + 4]
	int 0x80
	ret 4
syscall_print_stub_size: dd syscall_print_stub_size - syscall_print_stub

syscall_exit_stub:
	mov eax, 1
	mov ebx, [esp + 4]
	int 0x80
	ret 4
syscall_exit_stub_size: dd syscall_exit_stub_size - syscall_exit_stub

syscall_pos_cursor_stub:
	mov eax, 2
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	int 0x80
	ret 8
syscall_pos_cursor_stub_size: dd syscall_pos_cursor_stub_size - syscall_pos_cursor_stub

syscall_clear_screen_stub:
	mov eax, 3
	int 0x80
	ret
syscall_clear_screen_stub_size: dd syscall_clear_screen_stub_size - syscall_clear_screen_stub

syscall_execve_stub:
	mov eax, 4
	mov ebx, [esp + 4]
	int 0x80
	ret 4
syscall_execve_stub_size: dd syscall_execve_stub_size - syscall_execve_stub

syscall_fork_stub:
	mov eax, 5
	int 0x80
	ret
syscall_fork_stub_size: dd syscall_fork_stub_size - syscall_fork_stub
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
global syscall_open_stub
global syscall_open_stub_size
global syscall_read_stub
global syscall_read_stub_size
global syscall_write_stub
global syscall_write_stub_size
global syscall_close_stub
global syscall_close_stub_size

; NOTE: syscall stubs are using stdcall for now
; @TODO: ebx can't be destroyed in stdcall

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

syscall_open_stub:
	mov eax, 6
	mov ebx, [esp + 4]
	int 0x80
	ret 4
syscall_open_stub_size: dd syscall_open_stub_size - syscall_open_stub

syscall_read_stub:
	mov eax, 7
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov edx, [esp + 12]
	int 0x80
	ret 12
syscall_read_stub_size: dd syscall_read_stub_size - syscall_read_stub

syscall_write_stub:
	mov eax, 8
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov edx, [esp + 12]
	int 0x80
	ret 12
syscall_write_stub_size: dd syscall_write_stub_size - syscall_write_stub

syscall_close_stub:
	mov eax, 9
	mov ebx, [esp + 4]
	int 0x80
	ret 4
syscall_close_stub_size: dd syscall_close_stub_size - syscall_close_stub
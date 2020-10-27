global paging_switch_page_directory
global paging_get_faulting_address
global paging_copy_frame

; enable paging and switch to page directory received as parameter
paging_switch_page_directory:
	mov eax, [esp + 4]
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	ret

; when we receive a page fault interrupt (INT 14), the processor puts the faulting address in register CR2
; this is a helper function that gets the value and returns in eax
paging_get_faulting_address:
	mov eax, cr2
	ret

; disables paging and copies one frame (4KB) to another
paging_copy_frame:
	push ebp
	mov ebp, esp
	push ebx
	pushf				; push EFLAGS. We restore them later, so if interrupts were enabled, we re-enable them
	cli					; disable interrupts
	mov eax, [ebp + 12]	; frame_src_addr
	mov ecx, [ebp + 8]	; frame_dst_addr

	mov edx, cr0
	and edx, 0x7FFFFFFF
	mov cr0, edx		; disable paging

	mov edx, 1024
paging_copy_frame_loop:
	mov ebx, [eax]
	mov [ecx], ebx 
	add ecx, 4
	add eax, 4
	dec edx
	jnz paging_copy_frame_loop

	mov edx, cr0
	or edx, 0x80000000
	mov cr0, edx

	popf
	pop ebx
	pop ebp
	ret
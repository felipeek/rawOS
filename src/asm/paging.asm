global paging_switch_page_directory
global paging_get_faulting_address

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
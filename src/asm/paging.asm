global paging_switch_page_directory

; enable paging and switch to page directory received as parameter
paging_switch_page_directory:
push ebp
mov ebp, esp

mov eax, [ebp + 8]
mov cr3, eax
mov eax, cr0
or eax, 0x80000000
mov cr0, eax

pop ebp
ret
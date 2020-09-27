; This will be the first loaded content by the bootstrap process, so it is the kernel entrypoint.
; We just call the main function
[bits 32]
[extern main]
mov ebp, 0xC0000000                ; Change kernel stack to 0xC0000000
mov esp, ebp                       ; NOTE: If this value is changed, the constant in paging.c needs to be changed aswell.
call main
jmp $
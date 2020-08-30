; This will be the first loaded content by the bootstrap process, so it is the kernel entrypoint.
; We just call the main function
[bits 32]
[extern main]
call main
jmp $
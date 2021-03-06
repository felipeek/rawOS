global process_switch_context
global process_switch_context_magic_return_value
global process_switch_to_user_mode
global process_switch_to_user_mode_set_stack_and_jmp_addr
global process_flush_tlb

; Set the context for the new process.
; the new EIP, ESP and EBP are received as parameters and are set accordingly.
; ESP and EBP are just set to their corresponding registers
; EIP is stored in ebx and, at the end of the function, we jump to ebx.
; We also set the page directory of the new process.
;
; NOTE: Before returning, we set EAX to a magic return value.
; This is needed because we are jumping to a place in which we don't know if we are in the context of
; the process that is losing the CPU or the one receiving the CPU (see process.c).
; For this reason, we force eax to this magic number. So, in this function, we know that we are the process
; that is receiving the CPU if EAX is set to this magic number.
;
; NOTE: Even though we are using cdecl, this function is modifying ebx.
; However, since we are jumping to a new context, we will never return back to the caller.
; Therefore, we can safely destroy ebx.
;
; void process_switch_context(u32 eip, u32 esp, u32 ebp, u32 page_directory_x86_tables_frame_addr)
process_switch_context:
	cli
	mov ebx, [esp + 4]	; ebx = eip
	mov eax, [esp + 8]	; eax = esp
	mov ecx, [esp + 12]	; ecx = ebp
	mov edx, [esp + 16]	; edx = page_directory_x86_tables_frame_addr
	mov esp, eax
	mov ebp, ecx
	mov cr3, edx
	mov eax, [process_switch_context_magic_return_value]
	sti
	jmp ebx

process_switch_context_magic_return_value: dd 0x12345678

; This function just forces the switch to user-mode.
; The current stack is used as the user-mode stack.
; The jmp address is set to the end of the function, so we return to the caller.
; NOTE: interruptions are re-enabled when switching to user-mode
; void process_switch_to_user_mode()
process_switch_to_user_mode:
	cli
	mov ax, 0x23	; 0x23 is the offset of our user-mode data gdt entry, with last two bits set to 1, to indicate RPL 3.
	mov ds, ax		; set all segment selectors
	mov es, ax
	mov fs, ax
	mov gs, ax

	; now, we need to set up our stack in the way 'iret' expects
	mov eax, esp						; store our current esp in eax
	push 0x23							; 0x23 has same meaning here (user-mode data gdt entry)
	push eax							; push eax (which contains the old esp). So our stack is restored after 'iret'

	; we need to push the processor flags, so they are restored after iret
	; however, here we wanna mutate the flags so interrupts are enabled, because once we
	; return from iret, we will not be able to call 'sti' (because we will be in user-mode)
	pushf
	pop eax
	or eax, 0x200
	push eax

	push 0x1b								; 0x1b is the offset of our user-mode code gdt entry, with last two bits set to 1, to indicate RPL 3.
	push process_switch_to_user_mode_exit	; push the address in which we wanna return after iret
	iret
process_switch_to_user_mode_exit:
	ret

; This function forces the switch to user-mode.
; It also receives as parameters the desired user-mode stack and jump address.
; When the switch is done, the received stack is set.
; Also, the processor will jump to the received address.
; Therefore, this function does not return to the caller.
; NOTE: interruptions are re-enabled when switching to user-mode
; void process_switch_to_user_mode_set_stack_and_jmp_addr(u32 esp, u32 addr);
process_switch_to_user_mode_set_stack_and_jmp_addr:
	cli
	mov ax, 0x23	; 0x23 is the offset of our user-mode data gdt entry, with last two bits set to 1, to indicate RPL 3.
	mov ds, ax		; set all segment selectors
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov eax, [esp + 4]						; eax now contains the stack pointer received as a parameter
	mov ecx, [esp + 8]						; ecx now contains the jmp address received as a parameter

	; now, we need to set up our stack in the way 'iret' expects
	push 0x23								; 0x23 has same meaning here (user-mode data gdt entry)
	push eax								; push eax (which contains the desired esp). So our stack is restored after 'iret'

	; we need to push the processor flags, so they are restored after iret
	; however, here we wanna mutate the flags so interrupts are enabled, because once we
	; return from iret, we will not be able to call 'sti' (because we will be in user-mode)
	pushf
	pop eax
	or eax, 0x200
	push eax

	push 0x1b							; 0x1b is the offset of our user-mode code gdt entry, with last two bits set to 1, to indicate RPL 3.
	push ecx							; push the address in which we wanna return after iret, which is received as a parameter
	iret

process_flush_tlb:
	mov eax, cr3
	mov cr3, eax
	ret
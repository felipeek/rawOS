global process_switch_context

; Here we:
; * Stop interrupts so we don't get interrupted.
; * Temporarily put the new EIP location in ECX.
; * Load the stack and base pointers from the new task struct.
; * Change page directory to the physical address (physicalAddr) of the new directory.
; * Put a dummy value (0x12345) in EAX so that above we can recognise that we've just
; switched task.
; * Restart interrupts. The STI instruction has a delay - it doesn't take effect until after
; the next instruction.
; * Jump to the location in ECX (remember we put the new EIP in there).
; we are modifying ebx, but since we are jumping to a new context, we will never return back to the caller,
; so we can destroy ebx safely
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
	mov eax, 0x12345
	sti
	jmp ebx

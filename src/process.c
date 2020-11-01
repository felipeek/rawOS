#include "process.h"
#include "asm/interrupt.h"
#include "paging.h"
#include "alloc/kalloc.h"
#include "asm/util.h"
#include "util/printf.h"
#include "asm/process.h"
#include "gdt.h"
#include "asm/paging.h"
#include "util/util.h"
#include "rawx.h"
#include "fs/vfs.h"

#define PROCESS_KERNEL_STACK_SIZE 2 * 1024

typedef struct Process {
	u32 pid;
	u32 esp;							// process stack pointer
	u32 ebp;							// process stack base pointer
	u32 eip;							// process instruction pointer
	Page_Directory* page_directory;		// the page directory of this process
	
	// For safety, we allocate a separate kernel stack per process to treat syscalls.
	// For now I don't think our scheduler can switch the process context when a syscall is being treated
	// but we are doing this anyway.
	u32 kernel_stack;

	struct Process* next;
} Process;

static u32 current_pid = 1;
static Process* process_queue;
static Process* current_process;

void process_init() {
	// We start by disabling interrupts
	interrupt_disable();

	Vfs_Node* rawx_node = vfs_lookup(vfs_root, "out3.rawx");
	util_assert("Unable to find rawx of root process", rawx_node != 0);

	// Here we need to load the bash process and start it.
	// For now, let's load a fake process.
	current_process = kalloc_alloc(sizeof(Process));
	process_queue = current_process;
	current_process->pid = current_pid++;
	// Create kernel stack for process
	current_process->kernel_stack = (u32)kalloc_alloc_aligned(PROCESS_KERNEL_STACK_SIZE, 0x4);
	// For now let's clone the address space of the kernel.
	current_process->page_directory = paging_clone_page_directory_for_new_process(paging_get_kernel_page_directory());

	u32 addr = paging_get_page_directory_x86_tables_frame_address(current_process->page_directory);

	// NOTE(felipeek): IMPORTANT!
	// This will modify the stack to the state it was inside the 'paging_clone_page_directory_for_new_process'
	// function, because the address space was cloned inside that function... and the stack was copied, not linked.
	// This works because:
	// 1 - The function that will be called, 'paging_switch_page_directory', guarantees that it will be able to return even if
	// the address space switch modifies the stack.
	// 2 - Even though the stack will change, the stack frame set by this current function (process_init) is still valid, because
	// the address space was copied inside a function that was called by this function. For this reason, since the 'esp' and 'ebp'
	// pointers are NOT restored as part of the address space switch, we will continue pointing to the correct place in the stack,
	// and it should be still valid and contain our values. The only thing that can mess up this process is if the stack held by this
	// function (process_init) is modified BETWEEN the page directory cloning and the address space switch. In this case, we would
	// lose the changes. For this reason, we need to modify one immediately after another, and avoid modifying stack values.
	// Note that the value of 'addr' is lost after the address-space switch for this reason :)
	paging_switch_page_directory(addr);

	u8* buffer = kalloc_alloc(rawx_node->size);
	vfs_read(rawx_node, 0, rawx_node->size, buffer);
	RawOS_Header* rawx_header = rawx_load(buffer, rawx_node->size, current_process->page_directory);

	util_assert("stack size must be greater than 0", rawx_header->stack_size > 0);
	u32 stack_addr = 0xC0000000;

	// @NOTE: for this first process, we dont need to create the stack. We simply use the pages of the old kernel stack,
	// which were copied to the new address space
	//u32 stack_pages = rawx_header->stack_size / 0x1000 + 1;
	//for (u32 i = 0; i < stack_pages; ++i) {
	//	u32 page_num = (stack_addr / 0x1000) - i;
	//	printf("jaisda\n");
	//	paging_create_page_with_any_frame(current_process->page_directory, page_num);
	//}

	current_process->ebp = stack_addr;
	current_process->esp = stack_addr;
	current_process->eip = rawx_header->load_address + rawx_header->entry_point_offset;
	current_process->next = 0;

	gdt_set_kernel_stack(current_process->kernel_stack);
	//printf("jasda: %x\n", current_process->eip);

	// NOTE: interrupts will be re-enabled automatically by this function once we jump to user-mode.
	// Here, we basically force the switch to user-mode and we tell the processor to use the
	// stack defined by current_process->esp and to jump to the address defined by current_process->eip.
	process_switch_to_user_mode_set_stack_and_jmp_addr(current_process->esp, current_process->eip);
}

/*
void process_init() {
	// We start by disabling interrupts
	interrupt_disable();

	// Here we need to load the bash process and start it.
	// For now, let's load a fake process.
	current_process = kalloc_alloc(sizeof(Process));
	process_queue = current_process;
	current_process->pid = current_pid++;
	// Create kernel stack for process
	current_process->kernel_stack = (u32)kalloc_alloc_aligned(PROCESS_KERNEL_STACK_SIZE, 0x4);
	// For now let's clone the address space of the kernel.
	current_process->page_directory = paging_clone_page_directory_for_new_process(paging_get_kernel_page_directory());

	u32 addr = paging_get_page_directory_x86_tables_frame_address(current_process->page_directory);

	// NOTE(felipeek): IMPORTANT!
	// This will modify the stack to the state it was inside the 'paging_clone_page_directory_for_new_process'
	// function, because the address space was cloned inside that function... and the stack was copied, not linked.
	// This works because:
	// 1 - The function that will be called, 'paging_switch_page_directory', guarantees that it will be able to return even if
	// the address space switch modifies the stack.
	// 2 - Even though the stack will change, the stack frame set by this current function (process_init) is still valid, because
	// the address space was copied inside a function that was called by this function. For this reason, since the 'esp' and 'ebp'
	// pointers are NOT restored as part of the address space switch, we will continue pointing to the correct place in the stack,
	// and it should be still valid and contain our values. The only thing that can mess up this process is if the stack held by this
	// function (process_init) is modified BETWEEN the page directory cloning and the address space switch. In this case, we would
	// lose the changes. For this reason, we need to modify one immediately after another, and avoid modifying stack values.
	// Note that the value of 'addr' is lost after the address-space switch for this reason :)
	paging_switch_page_directory(addr);

	// @TEMPORARY: Deploy the following test code:
	// int 0x80
	// jmp $
	u32 code_addr = 0x40000000;
	u32 code_page_num = code_addr / 0x1000;
	u32 stack_addr = 0x60000000;
	u32 stack_page_num = stack_addr / 0x1000;
	paging_create_page_with_any_frame(current_process->page_directory, code_page_num);
	paging_create_page_with_any_frame(current_process->page_directory, stack_page_num);
	paging_create_page_with_any_frame(current_process->page_directory, stack_page_num - 1);
	paging_create_page_with_any_frame(current_process->page_directory, stack_page_num - 2);
	u8 infinite_jmp[2];
	infinite_jmp[1] = 0xFE;
	infinite_jmp[0] = 0xEB;
	u8 interruption[2];
	interruption[1] = 0x80;
	interruption[0] = 0xCD;
	util_memcpy((u32*)code_addr, interruption, 2);
	util_memcpy((u32*)((u8*)code_addr + 2), infinite_jmp, 2);

	current_process->ebp = stack_addr;
	current_process->esp = stack_addr;
	current_process->eip = code_addr;
	current_process->next = 0;

	gdt_set_kernel_stack(current_process->kernel_stack);

	// NOTE: interrupts will be re-enabled automatically by this function once we jump to user-mode.
	// Here, we basically force the switch to user-mode and we tell the processor to use the
	// stack defined by current_process->esp and to jump to the address defined by current_process->eip.
	process_switch_to_user_mode_set_stack_and_jmp_addr(current_process->esp, current_process->eip);
}
*/

s32 process_fork() {
	interrupt_disable();

	Process* new_process = kalloc_alloc(sizeof(Process));

	// For now, just add to the end of the queue.
	Process* aux = process_queue;
	while (aux->next) {
		aux = aux->next;
	}
	aux->next = new_process;

	// We get the current eip. This eip will be assigned to the new process.
	u32 eip = util_get_eip();

	// Note that from here we have two contexts: we are either the parent process or we are the child (newly created process).
	// If we are the parent process, we should continue with the child creation.
	// If we are the child, it means that we were just invoked by the scheduler for the first time. So, we shall
	// avoid the creation (which was obviously already done), and just get out of here.

	if (current_process != new_process) {
		// We are the parent, so we continue creating the child process

		// Create kernel stack for process
		new_process->kernel_stack = (u32)kalloc_alloc_aligned(PROCESS_KERNEL_STACK_SIZE, 0x4);
		// Clone our page directory for the child
		new_process->page_directory = paging_clone_page_directory_for_new_process(current_process->page_directory);
		// Set the pid of the child
		new_process->pid = current_pid++;
		// Set the EIP of the child to 'eip'. Note that this was obtained above via 'util_get_eip()'.
		// So, when this child is invoked, we will be jumping to the next instruction after the 'util_get_eip()' call.
		new_process->eip = eip;
		// Set ESP and EBP to the current ESP/EBP.
		asm volatile("mov %%esp, %0" : "=r"(new_process->esp));
		asm volatile("mov %%ebp, %0" : "=r"(new_process->ebp));
		interrupt_enable();
		// We return the pid of the child to indicate to the caller that he is in the parent context, just like UNIX does.
		return new_process->pid;
	} else {
		// We are the child, so we return 0 to indicate to the caller that he is in the child context.
		return 0;
	}
}

void process_switch() {
	if (!current_process) {
		return;
	}

	// We want to switch processes. So, before invoking the new process, we need to store the information about the process that
	// is currently running. For that, the first thing we do is get our current EIP. So we can set this EIP for the active process.
	// Note that when the process that is currently active is invoked again, he will start from here (just after util_get_eip).
	u32 eip = util_get_eip();

	// Note that from here we have two contexts: we are either a process that is losing the CPU
	// or we are a process that is receiving the CPU.
	// If we are the process that is losing the CPU, we need to continue the context switching normally.
	// If we are the process that is receiving the CPU, we just return.

	if (eip == process_switch_context_magic_return_value) {
		// When eip == magic_value, we know that we are the process that is receiving the CPU.
		// The magic value is set in 'process_switch_context' implementation.
		return;
	}

	// From now on, we know that we are the process that is losing the CPU.

	// Store our EIP, EBP and ESP.
	current_process->eip = eip;
	asm volatile("mov %%esp, %0" : "=r"(current_process->esp));
	asm volatile("mov %%ebp, %0" : "=r"(current_process->ebp));

	// Change 'current_process' to the next one.
	current_process = current_process->next;
	if (!current_process) {
		current_process = process_queue;
	}

	// Get the frame address of the page directory of the new process
	u32 page_directory_x86_tables_frame_addr = paging_get_page_directory_x86_tables_frame_address(current_process->page_directory);

	gdt_set_kernel_stack(current_process->kernel_stack + PROCESS_KERNEL_STACK_SIZE);

	// Finally, switch the context.
	process_switch_context(current_process->eip, current_process->esp, current_process->ebp, page_directory_x86_tables_frame_addr);
}
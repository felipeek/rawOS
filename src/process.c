#include "process.h"
#include "asm/interrupt.h"
#include "paging.h"
#include "alloc/kalloc.h"
#include "asm/util.h"
#include "util/printf.h"
#include "asm/process.h"
#include "gdt.h"

typedef struct Process {
	u32 pid;
	u32 esp;							// process stack pointer
	u32 ebp;							// process stack base pointer
	u32 eip;							// process instruction pointer
	Page_Directory* page_directory;		// the page directory of this process
	struct Process* next;
} Process;

static u32 current_pid = 1;
static Process* process_queue;
static Process* current_process;

void process_init() {
	interrupt_disable();

	// We start the process module by creating a process that represents the kernel.
	// This is basically the "process" that is currently running, i.e., it represents our current context.
	current_process = (Process*)kalloc_alloc(sizeof(Process));
	// pid 1 is being used to this kernel process
	current_process->pid = current_pid++;
	// ESP, EBP and EIP can be set to 0, because we don't need to immediately switch to this process
	// (since we are already this process)
	// When we lose the CPU, then ESP, EBP and EIP will be set accordingly.
	current_process->esp = 0;
	current_process->ebp = 0;
	current_process->eip = 0;
	// The page directory of the kernel process is the kernel page directory.
	current_process->page_directory = paging_get_kernel_page_directory();
	current_process->next = 0;
	process_queue = current_process;

	interrupt_enable();
}

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

	// Finally, switch the context.
	process_switch_context(current_process->eip, current_process->esp, current_process->ebp, page_directory_x86_tables_frame_addr);
}
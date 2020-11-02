#include "process.h"
#include "asm/interrupt.h"
#include "paging.h"
#include "alloc/kalloc.h"
#include "asm/util.h"
#include "util/printf.h"
#include "asm/process.h"
#include "asm/paging.h"
#include "util/util.h"
#include "rawx.h"
#include "fs/vfs.h"

#define INITIAL_PROCESS "rawos_print.rawx"

typedef struct Process {
	u32 pid;
	u32 esp;							// process stack pointer
	u32 ebp;							// process stack base pointer
	u32 eip;							// process instruction pointer
	Page_Directory* page_directory;		// the page directory of this process

	struct Process* next;
} Process;

static u32 current_pid = 1;
// NOTE: for now 'process_list' must contain a reference to ALL processes, since we rely on it
// (e.g. process_link_kernel_table_to_all_address_spaces)
static Process* process_list = 0;
Process* active_process = 0;

void process_init() {
	// We start by disabling interrupts
	interrupt_disable();

	Vfs_Node* rawx_node = vfs_lookup(vfs_root, INITIAL_PROCESS);
	util_assert("Unable to find rawx of root process", rawx_node != 0);

	// Here we need to load the bash process and start it.
	// For now, let's load a fake process.
	active_process = kalloc_alloc(sizeof(Process));
	process_list = active_process;
	active_process->next = 0;
	active_process->pid = current_pid++;
	// For now let's clone the address space of the kernel.
	active_process->page_directory = paging_clone_page_directory_for_new_process(paging_get_kernel_page_directory());

	u32 addr = paging_get_page_directory_x86_tables_frame_address(active_process->page_directory);

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

	// @NOTE: for this first process, we dont need to create the stack. We simply use the pages of the old kernel stack,
	// which were copied to the new address space
	RawX_Load_Information rli = rawx_load(buffer, rawx_node->size, active_process->page_directory, 0);
	u32 stack_addr = KERNEL_STACK_ADDRESS;

	active_process->ebp = stack_addr;
	active_process->esp = stack_addr;
	active_process->eip = rli.entrypoint;

	// NOTE: interrupts will be re-enabled automatically by this function once we jump to user-mode.
	// Here, we basically force the switch to user-mode and we tell the processor to use the
	// stack defined by active_process->esp and to jump to the address defined by active_process->eip.
	process_switch_to_user_mode_set_stack_and_jmp_addr(active_process->esp, active_process->eip);
}

s32 process_fork() {
	Process* new_process = kalloc_alloc(sizeof(Process));

	// For now, just add to the end of the queue.
	Process* aux = process_list;
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

	if (active_process != new_process) {
		// We are the parent, so we continue creating the child process

		// Clone our page directory for the child
		new_process->page_directory = paging_clone_page_directory_for_new_process(active_process->page_directory);

		// Create kernel stack for process
		// We copy the current kernel stack to the new kernel stack.
		// This is needed because when the new process is invoked, we wanna have the exact same kernel stack that we have right now.
		for (u32 i = 0; i < RAWX_KERNEL_STACK_RESERVED_PAGES_IN_PROCESS_ADDRESS_SPACE; ++i) {
			u32 page_num = (RAWX_KERNEL_STACK_ADDRESS_IN_PROCESS_ADDRESS_SPACE / 0x1000) - i;
			u32 frame_dst_addr = paging_get_page_frame_address(new_process->page_directory, page_num);
			u32 frame_src_addr = paging_get_page_frame_address(active_process->page_directory, page_num);
			paging_copy_frame(frame_dst_addr, frame_src_addr);
		}

		// Set the pid of the child
		new_process->pid = current_pid++;
		// Set the EIP of the child to 'eip'. Note that this was obtained above via 'util_get_eip()'.
		// So, when this child is invoked, we will be jumping to the next instruction after the 'util_get_eip()' call.
		new_process->eip = eip;
		// Set ESP and EBP to the current ESP/EBP.
		asm volatile("mov %%esp, %0" : "=r"(new_process->esp));
		asm volatile("mov %%ebp, %0" : "=r"(new_process->ebp));
		new_process->next = 0;
		// We return the pid of the child to indicate to the caller that he is in the parent context, just like UNIX does.
		return new_process->pid;
	} else {
		// We are the child, so we return 0 to indicate to the caller that he is in the child context.
		return 0;
	}
}

void process_switch() {
	if (!active_process) {
		return;
	}

	if (!process_list->next) {
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
	active_process->eip = eip;
	asm volatile("mov %%esp, %0" : "=r"(active_process->esp));
	asm volatile("mov %%ebp, %0" : "=r"(active_process->ebp));

	// Change 'active_process' to the next one.
	active_process = active_process->next;
	if (!active_process) {
		active_process = process_list;
	}

	// Get the frame address of the page directory of the new process
	u32 page_directory_x86_tables_frame_addr = paging_get_page_directory_x86_tables_frame_address(active_process->page_directory);

	// Finally, switch the context.
	process_switch_context(active_process->eip, active_process->esp, active_process->ebp, page_directory_x86_tables_frame_addr);
}

void process_link_kernel_table_to_all_address_spaces(u32 page_table_virtual_address, u32 page_table_index, u32 page_table_x86_representation) {
	// For now, just add to the end of the queue.
	Process* current_process = process_list;
	if (!current_process) {
		return;
	}

	while (current_process->next) {
		Page_Directory* current_process_page_directory = current_process->page_directory;
		current_process_page_directory->tables[page_table_index] = (Page_Table*)page_table_virtual_address;
		current_process_page_directory->tables_x86_representation[page_table_index] = page_table_x86_representation;
		current_process = current_process->next;
	}
}
#include "process.h"
#include "asm/interrupt.h"
#include "paging.h"
#include "alloc/kalloc.h"
#include "asm/util.h"
#include "util/printf.h"
#include "asm/process.h"

typedef struct Process {
	u32 pid;
	u32 esp;
	u32 ebp;
	u32 eip;
	Page_Directory* page_directory;
	struct Process* next;
} Process;

static u32 current_pid = 1;
static Process* process_queue;
static Process* current_process;

void process_init() {
	interrupt_disable();

	current_process = (Process*)kalloc_alloc(sizeof(Process));
	current_process->pid = current_pid++;
	current_process->esp = 0;
	current_process->ebp = 0;
	current_process->eip = 0;
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

	// Note that from here we have two contexts: we are the parent process or we are the child (newly created process).
	// If we are the parent process, we should continue with the child creation.
	// If we are the child, it means that we were just invoked by the scheduler for the first time. So, we shall
	// avoid the creation (which was obviously already done), and just get out of here.

	if (current_process != new_process) {
		new_process->page_directory = paging_clone_page_directory_for_new_process(current_process->page_directory);
		new_process->pid = current_pid++;
		new_process->eip = eip;
		asm volatile("mov %%esp, %0" : "=r"(new_process->esp));
		asm volatile("mov %%ebp, %0" : "=r"(new_process->ebp));
		interrupt_enable();
		return new_process->pid;
	} else {
		return 0;
	}
}

void process_switch() {
	if (!current_process) {
		return;
	}

	u32 eip = util_get_eip();

	if (eip == 0x12345) {
		return;
	}

	current_process->eip = eip;
	asm volatile("mov %%esp, %0" : "=r"(current_process->esp));
	asm volatile("mov %%ebp, %0" : "=r"(current_process->ebp));

	current_process = current_process->next;
	if (!current_process) {
		current_process = process_queue;
	}

	u32 page_directory_x86_tables_frame_addr = paging_get_page_directory_x86_tables_frame_address(current_process->page_directory);
	process_switch_context(current_process->eip, current_process->esp, current_process->ebp, page_directory_x86_tables_frame_addr);
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>

#include <sys/ptrace.h>
#include <sys/user.h> // user_regs_struct 
#include <linux/elf.h> // NT_PRSTATUS
#include <sys/uio.h> // struct iovec

struct user_regs_struct regs;
struct iovec pt_iov = {
	.iov_base = &regs,
	.iov_len = sizeof(regs),
};

long insert_breakpoint(pid_t pid, void *addr)
{
	long retval, instr_backup;

	/* Look at the word at the address we're interested in */
	instr_backup = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	assert(-1 != instr_backup);
	printf("tracer: instruction at %p is: 0x%016lx\n", addr, instr_backup);

	/* Replace the original instruction with the trap instruction 'int3' */
	printf("tracer: inserting trap\n");
	long instr_with_trap = (instr_backup & ~0xFF) | 0xCC;
	retval = ptrace(PTRACE_POKETEXT, pid, addr, (void*)instr_with_trap);
	assert(-1 != retval);

	/* See what's there again... */
	retval = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	assert(-1 != retval);
	printf("tracer: instruction at %p is: 0x%016lx\n", addr, retval);

	return instr_backup;
}

void restore_breakpoint(pid_t pid, void *addr, long data)
{
	/* 
	 * Remove the breakpoint by restoring the previous data
	 * at the target address, and unwind the RIP back by 1 to 
	 * let the CPU execute the original instruction that was 
	 * there.
	 */
	long retval;

	printf("tracer: restoring ...\n");
	retval = ptrace(PTRACE_POKETEXT, pid, (void*)addr, (void*)data);
	assert(-1 != retval);
	
	/* See what's there again... */
	retval = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
	assert(-1 != retval);
	printf("tracer: instruction at %p is: 0x%016lx\n", addr, retval);

	regs.rip -= 1;
	retval = ptrace(PTRACE_SETREGSET, pid, NT_PRSTATUS, &pt_iov);
	assert(-1 != retval);
}

int main(void)
{
	int wstatus;
	long retval;
	pid_t pid;

	pid = fork();
	assert(-1 != pid);

	if (0 == pid) {
		retval = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		assert(-1 != retval);

		retval = execl("./tracee", "tracee", NULL);
		assert(-1 != retval);
	}

	long instr_backup;

	retval = waitpid(pid, &wstatus, 0);
	assert(-1 != retval);

	/* 
	 * user input break at syscall, which address is at 0x40101c
	 * i.e. "b *0x40101c"
	 * insert_breakpoint will backup the instruction at specified
	 * position and return it as instr_backup
	 */
	instr_backup = insert_breakpoint(pid, (void *)0x40101c);

	// continue
	retval = ptrace(PTRACE_CONT, pid, NULL, NULL);
	assert(-1 != retval);

	retval = waitpid(pid, &wstatus, 0);
	assert(-1 != retval);

	assert(WIFSTOPPED(wstatus));
	printf("tracer: tracee got a signal and was stopped: %s\n", strsignal(WSTOPSIG(wstatus)));

	/* See where the tracee is now */
	retval = ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &pt_iov);
	assert(-1 != retval);
	printf("tracer: tracee is stopped at IP = %p, breakpoint take effective!\n", (void *)regs.rip);

	// now the breakpoint is stopped.

	// if user input "continue"
	// restore the origin instructive at breakpoint
	restore_breakpoint(pid, (void *)0x40101c, instr_backup);

	printf("tracer: continue the tracee\n");
	retval = ptrace(PTRACE_CONT, pid, NULL, NULL);
	assert(-1 != retval);

	return 0;
}

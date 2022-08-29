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

void info_registers(pid_t pid)
{
	long retval;
	struct user_regs_struct regs;
	struct iovec pt_iov = {
		.iov_base = &regs,
		.iov_len = sizeof(regs),
	};
	retval = ptrace(PTRACE_GETREGSET, pid, NT_PRSTATUS, &pt_iov);
	assert(-1 != retval);

	// just print some important registers we care about
	printf( "--------------------------\n"
		"rax: 0x%llx\n"
		"rdi: 0x%llx\n"
		"rsi: 0x%llx\n"
		"rdx: 0x%llx\n",
		regs.rax,
		regs.rdi,
		regs.rsi,
		regs.rdx);
}

int main(void)
{
	int wstatus;
	long retval;
	pid_t pid;

	pid = fork();
	assert(-1 != pid);

	if (0 == pid) {
		// child process starts ......
		retval = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		assert(-1 != retval);

		// a successful call of execl will make tracee received
		// a SIGTRAP, which gives the parent tracer a chance to 
		// gain control before the new program begins execution.
		retval = execl("./tracee", "tracee", NULL);
		assert(-1 != retval);
	}

	// parent process goes here

	// wait no-timeout till status of child is changed
	retval = waitpid(pid, &wstatus, 0);
	assert(-1 != retval);
	
	assert(WIFSTOPPED(wstatus));
	printf("tracer: tracee got a signal and was stopped: %s\n", strsignal(WSTOPSIG(wstatus)));

	printf("tracer: request to query more information from the tracee\n");
	info_registers(pid);
	
	// continue the tracee
	printf("tracer: request tracee to continue ...\n");
	retval = ptrace(PTRACE_CONT, pid, NULL, NULL);
	assert(-1 != retval);

	return 0;
}
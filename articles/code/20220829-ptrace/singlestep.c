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
	long counter = 0;
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

	while (1) {
		wait(&wstatus);
		
		if (WIFEXITED(wstatus)) {
			break;
		}

		info_registers(pid);
		counter++;

		retval = ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
		assert(-1 != retval);
	}

	printf( "==========================\n");
	printf("Number of machine instructions : %ld\n", counter);

	return 0;
}
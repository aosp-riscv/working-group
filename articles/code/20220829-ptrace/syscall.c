#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sys/ptrace.h>  // ptrace
#include <linux/ptrace.h> // struct ptrace_syscall_info

int main()
{
	pid_t pid;
	long retval;

	pid = fork();
	assert(-1 != pid);

	if (0 == pid) {
		retval = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		assert(-1 != retval);

		retval = execl("./tracee", "tracee", NULL);
		assert(-1 != retval);
	}

	struct ptrace_syscall_info syscall_info;
	long syscall_number;
	long args[6];
	long syscall_retval;

	retval = waitpid(pid, NULL, 0);
	assert(-1 != retval);

	// Set ptrace options
	// PTRACE_O_EXITKILL: make sure a SIGKILL signal is sent to the tracee if the tracer exit.
	// PTRACE_O_TRACESYSGOOD: required when tracing system calls
	retval = ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_EXITKILL|PTRACE_O_TRACESYSGOOD);
	assert(-1 != retval);

	retval = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	assert(-1 != retval);

	retval = waitpid(pid, NULL, 0);
	assert(-1 != retval);

	retval = ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(struct ptrace_syscall_info), &syscall_info);
	assert(-1 != retval);
	assert(PTRACE_SYSCALL_INFO_ENTRY == syscall_info.op);
	syscall_number = syscall_info.entry.nr;
	args[0] = syscall_info.entry.args[0];
	args[1] = syscall_info.entry.args[1];
	args[2] = syscall_info.entry.args[2];
	args[3] = syscall_info.entry.args[3];
	args[4] = syscall_info.entry.args[4];
	args[5] = syscall_info.entry.args[5];

	retval = ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
	assert(-1 != retval);

	retval = waitpid(pid, NULL, 0);
	assert(-1 != retval);

	retval = ptrace(PTRACE_GET_SYSCALL_INFO, pid, sizeof(struct ptrace_syscall_info), &syscall_info);
	assert(-1 != retval);
	assert(PTRACE_SYSCALL_INFO_EXIT == syscall_info.op);
	syscall_retval = syscall_info.exit.rval;

	printf("system call: "
		"number = %ld, "
		"arg[0] = %ld, arg[1] = %ld, arg[2] = %ld, "
		"return value = %ld\n",
		syscall_number,
		args[0], args[1], args[2],
		syscall_retval);

	return 0;
}

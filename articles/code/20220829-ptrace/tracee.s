	.section .rodata
msg:
	.string "Hello\n"

	.text
	.globl _start
_start:
	# write(int fd, const void *buf, size_t count);
	movq $1, %rax		# __NR_write
	movq $1, %rdi		# fd = 1
	leaq msg(%rip), %rsi	# buf = msg
	movq $6, %rdx		# count = 6
	syscall
	
	# exit(int status);
	movq $60, %rax		# __NR_exit
	xor %rdi, %rdi		# status = 0
	syscall

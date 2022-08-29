# riscv64-unknown-linux-gnu-gcc -nostdlib -fno-builtin tracee.s -o tracee -static -g -march=rv64imafd -mabi=lp64d

	.section .rodata
msg:
	.string "Hello\n"

	.text
	.globl _start
_start:
	# write(int fd, const void *buf, size_t count);
	li a7, 64	# __NR_write
	li a0, 1	# fd = 1
	la a1, msg	# buf = msg
	li a2, 6	# count = 6
	scall
	
	# exit(int status);
	li a7, 93
	li a0, 0
	scall

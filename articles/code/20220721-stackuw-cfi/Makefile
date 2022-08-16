CROSS_COMPILE = riscv64-unknown-linux-gnu-

.DEFAULT_GOAL := all
all: test-cfi

PATH_SYSROOT = /aosp/wangchen/test-gcc/install/sysroot/
SRCS = test.c foos.c backtrace.c
SRCS_NOCFI = test.c foos_no_cfi.S backtrace.c

.PHONY : test-cfi
test-cfi: clean
	${CROSS_COMPILE}gcc ${SRCS} -fomit-frame-pointer -fasynchronous-unwind-tables -rdynamic -o a.out -Wall
	${CROSS_COMPILE}gcc -S foos.c -fomit-frame-pointer -fasynchronous-unwind-tables
	${CROSS_COMPILE}gcc -c foos.c -fomit-frame-pointer -fasynchronous-unwind-tables

.PHONY : test-nocfi
test-nocfi: clean
	${CROSS_COMPILE}gcc ${SRCS_NOCFI} -fomit-frame-pointer -fasynchronous-unwind-tables -rdynamic -o a.out -Wall
	${CROSS_COMPILE}gcc -c foos_no_cfi.S -fomit-frame-pointer -fasynchronous-unwind-tables

.PHONY : run
run:
	qemu-riscv64 -L ${PATH_SYSROOT} a.out

.PHONY : dump
dump:
	${CROSS_COMPILE}objdump -d foos.o

.PHONY : clean
clean:
	rm -rf *o *.s a.out


CROSS_COMPILE = riscv64-unknown-linux-gnu-

.DEFAULT_GOAL := all
all: test

PATH_SYSROOT = /aosp/wangchen/test-gcc/install/sysroot/
SRCS = test.c backtrace.c

.PHONY : test
test: clean
	${CROSS_COMPILE}gcc ${SRCS} -fomit-frame-pointer -fasynchronous-unwind-tables -rdynamic -o a.out -Wall

.PHONY : run
run:
	qemu-riscv64 -L ${PATH_SYSROOT} a.out

.PHONY : clean
clean:
	rm -rf *.o *.s a.out


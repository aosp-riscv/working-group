CROSS_COMPILE = riscv64-unknown-linux-gnu-

.DEFAULT_GOAL := all
all: builtin

PATH_SYSROOT = /aosp/wangchen/test-gcc/install/sysroot/
PATH_LIBUNWIND_INCLUDE = /aosp/wangchen/test-libunwind/install/include
PATH_LIBUNWIND_LIB = /aosp/wangchen/test-libunwind/install/lib

SRCS_BUILTIN = test_builtin.c
SRCS_LIBUNWIND = libunwind.c test_libunwind.c
SRCS_LIBUNWIND_SIGNAL = libunwind.c test_libunwind_signal.c

.PHONY : builtin
builtin: clean
	${CROSS_COMPILE}gcc -fomit-frame-pointer -fexceptions -Wall -o a.out ${SRCS_BUILTIN}

.PHONY : libunwind
libunwind: clean
	${CROSS_COMPILE}gcc -fomit-frame-pointer -fexceptions -I ${PATH_LIBUNWIND_INCLUDE} -L ${PATH_LIBUNWIND_LIB} -lunwind-riscv -lunwind -Wall -o a.out ${SRCS_LIBUNWIND} -g

.PHONY : libunwind-signal
libunwind-signal: clean
	${CROSS_COMPILE}gcc -fomit-frame-pointer -fexceptions -I ${PATH_LIBUNWIND_INCLUDE} -L ${PATH_LIBUNWIND_LIB} -lunwind-riscv -lunwind -Wall -o a.out ${SRCS_LIBUNWIND_SIGNAL}

.PHONY : run
run:
	qemu-riscv64 -L ${PATH_SYSROOT} -E LD_LIBRARY_PATH=${PATH_LIBUNWIND_LIB} a.out

.PHONY : debug
debug:
	qemu-riscv64 -L ${PATH_SYSROOT} -E LD_LIBRARY_PATH=${PATH_LIBUNWIND_LIB} -g 1234 ./a.out &
	riscv64-unknown-linux-gnu-gdb -x gdbinit ./a.out


.PHONY : clean
clean:
	rm -rf *o *.s a.out


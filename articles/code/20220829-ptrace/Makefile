CROSS_COMPILE = 

.DEFAULT_GOAL := all
all: builtin

SRCS_BUILTIN = test_builtin.c
SRCS_LIBUNWIND = libunwind.c test_libunwind.c
SRCS_LIBUNWIND_SIGNAL = libunwind.c test_libunwind_signal.c

.PHONY : tracee
tracee: clean
	${CROSS_COMPILE}gcc -nostdlib -fno-builtin -static -o tracee tracee.s

.PHONY : tracer
tracer: tracee
	${CROSS_COMPILE}gcc -Wall -o a.out tracer.c
	./a.out

.PHONY : syscall
syscall: tracee
	${CROSS_COMPILE}gcc -Wall -o a.out syscall.c
	./a.out

.PHONY : breakpoint
breakpoint: tracee
	${CROSS_COMPILE}gcc -Wall -o a.out breakpoint.c
	./a.out

.PHONY : singlestep
singlestep: tracee
	${CROSS_COMPILE}gcc -Wall -o a.out singlestep.c
	./a.out

.PHONY : run
run:
	./a.out

.PHONY : clean
clean:
	rm -rf *.o a.out tracee


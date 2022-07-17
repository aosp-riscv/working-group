CROSS_COMPILE = riscv64-unknown-linux-gnu-

.DEFAULT_GOAL := all

callstack: clean
	${CROSS_COMPILE}gcc -g -c callstack.c

callstack-param: clean
	${CROSS_COMPILE}gcc -g -c param.c

callstack-o1: clean
	${CROSS_COMPILE}gcc -g -c callstack.c -O1

callstack-o1-fp: clean
	${CROSS_COMPILE}gcc -g -c callstack.c -O1 -fno-omit-frame-pointer

callstack-o2: clean
	${CROSS_COMPILE}gcc -g -c callstack.c -O2

callstack-o2-fp: clean
	${CROSS_COMPILE}gcc -g -c callstack.c -O2 -fno-omit-frame-pointer

callstack-o2-fp2: clean
	${CROSS_COMPILE}gcc -g -c callstack.c -O2 -fno-omit-frame-pointer -fno-optimize-sibling-calls

dump:
	${CROSS_COMPILE}objdump -S callstack.o

.PHONY : clean
clean:
	rm -rf *o


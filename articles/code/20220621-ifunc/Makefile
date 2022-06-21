CROSS_COMPILE = riscv64-unknown-linux-gnu-

MY_CONFIG ?= 1

.DEFAULT_GOAL := all

no-ifunc: clean
	${CROSS_COMPILE}gcc -g -shared -fPIC no_ifunc.c -o no_ifunc.so
	${CROSS_COMPILE}gcc -g -DMY_CONFIG=${MY_CONFIG} main.c no_ifunc.so -o a.out

ifunc: clean
	${CROSS_COMPILE}gcc -g -shared -fPIC -DIFUNC_PREEMPTIBLE ifunc.c -o ifunc.so
	${CROSS_COMPILE}gcc -g -DMY_CONFIG=${MY_CONFIG} main.c ifunc.so -o a.out

ifunc-static: clean
	${CROSS_COMPILE}gcc -g -shared -fPIC ifunc.c -o ifunc_static.so
	${CROSS_COMPILE}gcc -g -DMY_CONFIG=${MY_CONFIG} main.c ifunc_static.so -o a.out

.PHONY : clean
clean:
	rm -rf *.so a.out

.PHONY : run
run:
	@qemu-riscv64 -L /home/u/ws/test-gcc/install/sysroot/ -E LD_LIBRARY_PATH=. ./a.out

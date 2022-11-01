# General Purpose Makefile for Linux Kernel module

ARCH := riscv # set if cross-compile  
CROSS_COMPILE := riscv64-unknown-linux-gnu- # set if cross-compile

KERNEL_VER := $(shell uname -r)

# set to path of corresponding kernel source tree
KERNEL_DIR ?= /aosp/wangchen/dev-kernel/linux
		
ifneq ($(KERNELRELEASE),)
	# call from kernel build system
	obj-m := lkm_example.o
else

default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(CURDIR) modules

endif

clean:                                     
	$(MAKE) -C $(KERNEL_DIR) M=$(CURDIR) clean

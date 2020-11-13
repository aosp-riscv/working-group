# AOSP on RISC-V WG

THis is meta repo for AOSP for RISC-V Project. General tasks and issues here.

If you are interested in runnning Android on RISC-V hardware, please join us!

## Status

Currently we are in a very early stage, trying to cross compile AOSP codebase
using the RISC-V official GNU toolchain.

- 2020-08-15 We have added the lunch menu for RISC-V, and loaded the menu success.
After that, we added the RISC-V config files for build/make and build/soong,
we are debugging RISC-V the config files in build/soong.

- 2020-09-01 We fixed the errors in build dir and prebuilts dir when we using the
'mmm art/compiler' to generate the ninja files for compiler module. We are debugging
the errors in the system dir and cts dir.

- 2020-09-15 We launched task to porting AOSP kernel for RISC-V. After some research, we decided to base off the branch `android-5.4-stable` of AOSP common kernels and we are in working now.

- 2020-10-01 Using the latest AOSP kernel and configuration, with GNU and LLVM/Clang tool-chain for RISC-V, successfully compile and boot the kernel on QEMU. The only problem is: after mounting the minimum rootfs, encounter the "Segmentation Fault" error. Still in debugging this issue. In addition, We began to study how to port the bionic library of AOSP to RISC-V. The current work is to port the AOSP building framework for bionic.

- 2020-10-16 Simplify the build framework of the AOSP bionic library to make, and submit the preliminary porting work to the develop branch of <https://github.com/aosp-riscv/port_bionic>. Currently, the project repository depends on two sub-repositories <https://github.com/aosp-riscv/platform_bionic> and <https://github.com/aosp-riscv/external_jemalloc_new> (Because the changes are still draft, the current changes to aosp are all placed on their respective develop branches. Another note: the current porting work is based off `android-10.0.0_r39` tag version based on AOSP). The next step is to continue to improve the make project and make changes to the bionic code. The first goal is to implement `libc.a`.

- 2020-10-30 The make framework has been improved to support the compilation and generation of libc static libraries, as well as `crtbegin.o` and `crtend.o`. Currently, the statically compiled executable program can jump from the kernel to the main function and ensure that argv and envp are read correctly. The porting of other TLS and syscall parts is still in progress. For specific changes, please refer to the develop branch of <https://github.com/aosp-riscv/port_bionic>.

- 2020-11-13 Further improve the libc static library of bionic. Currently using v10.0.1 llvm/clang to compile, and then use the gnu riscv version of ld based on bionic's libc_static, crtbegin_static and crtend.o to achieve static linking and generate executable programs. On this basis, we transplanted and compiled the toybox that comes with AOSP and made a minimal rootfs (shell and init still use busybox compiled based on glibc). At present, the rootfs can be started and run on qemu-system-riscv64. You can run some commands, but some commands will report errors when executed. For specific changes, please refer to the commit comment of the develop branch of the bionic porting main repository <https://github.com/aosp-riscv/port_bionic> and the commit comment of the develop branch of its submodule repositories. The next step will continue to improve the libc of bionic and the self-made minimal root file system.

## About Us

The initial members of this project is from the [PLCT lab](https://github.com/isrc-cas/).
Wei Wu (@lazyparser) and Ningning Shi (@shining1984) co-found this project.

We would like to contribute all the efforts to RISC-V community and AOSP project.

## Join Us

If you want to contribute, please read the contributing guide from the AOSP website.

Feel free to open an issue for further questions/discussions!

## Learning Materials

### Slides

- [**Porting Android to new Platforms, Amit Pundir**](https://www.slideshare.net/linaroorg/porting-android-tonewplatforms)

### Articles

- [**Version Management for AOSP Platform, Wang Chen - PLCT lab, 20200911 (Chinese Version)**](https://zhuanlan.zhihu.com/p/234390474)
- [**Version Management for AOSP Kernel, Wang Chen - PLCT lab, 20200915 (Chinese Version)**](https://zhuanlan.zhihu.com/p/245131105)
- [**Running RISC-V 64 Linux on QEMU, Wang Chen - PLCT lab, 20200923 (Chinese Version)**](https://zhuanlan.zhihu.com/p/258394849)
- [**Compile Android Kenrel for RISC-V, Wang Chen - PLCT lab, 20200929 (Chinese Version)**](https://zhuanlan.zhihu.com/p/260356339)
- [**Make a LLVM/Clang compiler for RISC-V, Wang Chen - PLCT lab, 20201009 (Chinese Version)**](https://zhuanlan.zhihu.com/p/263550372)

### Vedios

- [**[COSCUP 2011] Porting android to brand-new CPU architecture, Luse Cheng (Traditional Chinese)**](https://www.youtube.com/watch?v=li6PqLn4Bl4)
- [**How difficult when do RISC-V porting for AOSP - Wu Wei - V8 technical symposium - OSDT community - 20200607 (Chinese Version)**](https://www.bilibili.com/video/BV1wC4y1a7Za)
- [**Introduction about Building framwork of AOSP and preliminary trying porting for RISC-V - Wang Chen - 20200805 - PLCT lab (Chinese Version)**](https://www.bilibili.com/video/BV1PA411Y7mz)
- [**AOSP for RISC-V porting tutorial (1) - Introduction about Android Runtime - Wang Chen - 20200814 - PLCT lab (Chinese Version)**](https://www.bilibili.com/video/BV1wC4y1t7Xa)
- [**AOSP for RISC-V porting tutorial (2) - Starting porting ART - Wang Chen - 20200821 - PLCT lab (Chinese Version)**](https://www.bilibili.com/video/BV1JK411M7e5)

### Discussions

- [**The status of AOSP porting: Is there anyone working on it (publicly)?**](https://groups.google.com/a/groups.riscv.org/g/sw-dev/c/u9iP7A2Wkc8)

### Other Resources 

- [**Android-x86**](https://www.android-x86.org/)

# AOSP on RISC-V WG

This is the meta repo for the AOSP for RISC-V Project. General tasks and issues here.

If you are interested in runnning Android on RISC-V hardware, please join us!

## Status

Currently we are in a very early stage, trying to cross compile the AOSP codebase
using the RISC-V official GNU toolchain. The following is the project progress log (time in reverse order).

- 2021-11-30 For AOSP 12 ported to RV64, the implementation of `mmm bionic/libc/ --skip-soong-tests` was successfully built, we now have libc/libm/libdl. The relevant changes have been submitted to the repositories. The details are as follows (here, some PRs are created on gitee, all commits are pushed both on github & gitee):
  - toolchain for AOSP：
    - Rust for Android:
      - compiler: add riscv64gc-linux-android target (Tier 3): <https://github.com/aosp-riscv/rust/commit/8397a76b895b586dcfc8637e98908f65df1af1f9>
      - library: add riscv64gc-linux-android support: <https://github.com/aosp-riscv/rust/commit/2eacf985415ba38018de70845c0b150885ad4d57>
      - Modified libc, added support for riscv64：<https://github.com/aosp-riscv/toolchain_rustc/pull/1>
    - Clang/llvm for Android:
      - add riscv __get_tls method: <https://github.com/aosp-riscv/toolchain_llvm-project/commit/2511f435f0c0c02f1a64d55b3b45c657dc84b050>
      - Change some repos' fetch address and revision to support riscv64: <https://github.com/aosp-riscv/platform_manifest/commit/a3446a1580947ac5d5d29e99365fdf90b0edb3ef>
  - manifest：PR: <https://gitee.com/aosp-riscv/platform_manifest/pulls/7>
  - AOSP (PRs)：
    - <https://gitee.com/aosp-riscv/platform-external-scudo/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-unwinding/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-libbase/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_build_soong/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_bionic/pulls/2>
    - <https://gitee.com/aosp-riscv/platform-external-kernel-headers/pulls>

- 2021-11-15 For AOSP 12 ported to RV64, the implementation of `m --skip-ninja --skip-soong-tests` was successfully built. The relevant changes have been submitted to the repositories. The details are as follows (here, gitee is taken as an example, github has been mirrored):
  - manifest repository：<https://gitee.com/aosp-riscv/platform_manifest>
  - Newly added AOSP repositories:
    - art/ : <https://gitee.com/aosp-riscv/platform_art>
    - bionic/: <https://gitee.com/aosp-riscv/platform_bionic>
    - build/make/: <https://gitee.com/aosp-riscv/platform_build>
    - build/soong/: <https://gitee.com/aosp-riscv/platform_build_soong>
    - external/crosvm/: <https://gitee.com/aosp-riscv/platform_external_crosvm>
    - frameworks/av/: <https://gitee.com/aosp-riscv/platform_frameworks_av>
    - packages/modules/NeuralNetworks/: <https://gitee.com/aosp-riscv/platform-packages-modules-NeuralNetworks>
    - prebuilts/vndk/v28/: <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v28>
    - prebuilts/vndk/v29/: <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v29>
    - prebuilts/vndk/v30/: <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v30>
    - system/apex/: <https://gitee.com/aosp-riscv/platform-system-apex>
    - system/core/: <https://gitee.com/aosp-riscv/platform_system_core>
    - system/iorap/: <https://gitee.com/aosp-riscv/platform-system-iorap>
    - system/nvram/: <https://gitee.com/aosp-riscv/platform-system-nvram>
  - In addition, some repositories are currently unable to be created on gitee/github because the overall volume or some files are too large and exceed the limitaiton required by gitee/github. Therefore, we currently use patching method to save the changes, and below are these repositories involved:
    - cts/
    - frameworks/base/
    - packages/modules/ArtPrebuilt/
    - packages/services/Car/
    - prebuilts/clang/host/linux-x86/
    - prebuilts/runtime/
    The newly built patch repository is: <https://gitee.com/aosp-riscv/patches_aosp_riscv>
  - The list of PRs submitted is as follows:
    - <https://gitee.com/aosp-riscv/platform_art/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_bionic/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_build/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_build_soong/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_external_crosvm/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_frameworks_av/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-packages-modules-NeuralNetworks/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_system_core/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_system_core/pulls/2>
    - <https://gitee.com/aosp-riscv/platform-system-iorap/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-nvram/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-apex/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v28/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v29/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v30/pulls/1>
    - <https://gitee.com/aosp-riscv/patches_aosp_riscv/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/6>
  - Updated tech article [“Howto build aosp-riscv” ](./articles/20211029-howto-setup-build-env.md)
  - Newed tech article ["Code analysis for soong_ui"](./articles/20211102-codeanalysis-soong_ui.md)

- 2021-10-29 Restart the AOSP migration work of PLCT lab. The new goal is to migrate AOSP 12 to RV64.
  - Setup group @ <https://github.com/aosp-riscv> (mirrored with <https://gitee.com/aosp-riscv>) with some initial repos.
  - [PR Merged] added note on howto setup build env: <https://gitee.com/aosp-riscv/working-group/pulls/1>
  - Newed tech article ["Code analysis for envsetup"](./articles/20211026-lunch.md)
  - Newed tech article ["Howto setup build environment"](./articles/20211029-howto-setup-build-env.md)

- 2021-01-xx Due to [open-source work](https://github.com/T-head-Semi/aosp-riscv) from T-Head(alibaba), we (PLCT lab) stopped the AOSP porting work in early 2021. All code repositories (except working-group) under the original <https://github.com/aosp-riscv> and <https://gitee.com/aosp-riscv> are backed up to [Gitee's aosp-riscv-bionic-porting](https://gitee.com/aosp-riscv-bionic-porting).

- 2021-01-15 Still in investigating how to add RVG64 support based on the Soong framework of AOSP. The main difficulty at present is: due to the AOSP compilation system is extremely large and complex, contains too much content required for engineering, and is intertwined and dependent on each other, so how to cleanly mask the modules that we don’t care about first and only compile those core content of the bionic library is the current focus and difficulty. There is not much clue yet, anyone is welcomed to provide help and suggestions.

- 2020-12-29 Completed the porting of dynamic linking feature for bionic, we can now support implicit dynamic link and explicit dynamic link with bionic. The make framework system is also improved. For specific changes, please refer to the `develop` branch of the bionic transplantation main repository, <https://github.com/aosp-riscv/port_bionic>. The next step is to port and modify AOSP's Soong construction system and use AOSP Soong construction system to compile the bionic library and related applications for RISC-V.

- 2020-12-15
    - Started to port the dynamic link functionality for bionic. Currently, the compilation and linking of `libc.so` and `linker` have been completed, but still can not work well. We are still under debugging. Anybody familiar with the dynamic linker are welcomed to provide your suggestions. For specific changes, please refer to the `develop` branch of the bionic transplantation main repository, <https://github.com/aosp-riscv/port_bionic>, and its corresponding submodule repository.
    - The focus of further work is still to achieve dynamic link support, and the current implementation flaws are felt to be larger than static links. In addition, we will continue to improve the bionic function and try to transplant the Soong construction system of AOSP, and support the use of AOSP Soong construction system to compile the bionic library and related applications for RISC-V.

- 2020-11-30 Completed the porting of shell and init. Currently, the bionic libc.a library can work, and a minimal Android root file system can be generated by statically linking against it. The rootfs can be launched and run on qemu-system-riscv64 and supports running some basic operation commands. In addition, the original make framework system has been improved to prepare for the next stage to further support dynamic linking. For specific changes, please refer to the `develop` branch of the bionic transplantation main repository, <https://github.com/aosp-riscv/port_bionic>, and its corresponding submodule repository. For more detailed introduction, please refer to another introduction article: ["Create a minimal Android system for RISC-V"] (https://plctlab.github.io/aosp/create-a-minimal-android-system-for-riscv.html). The next step is to improve the dynamic linking support, continue to improve the bionic function and try to transplant the Soong construction system of AOSP, and support the use of the Soong construction system of AOSP to compile the bionic library and related applications for RISC-V.

- 2020-11-13 Further improve the libc static library of bionic. Currently using v10.0.1 llvm/clang to compile, and using the GNU RISC-V version of ld based on bionic's libc_static, crtbegin_static and crtend.o to achieve static linking and executable program generation. On this basis, we transplanted and compiled the toybox that comes with AOSP and made a minimal rootfs (shell and init still use busybox compiled for glibc). At present, the rootfs can be started and run on qemu-system-riscv64. You can run some commands, but some commands will report errors when executed. For specific changes, please refer to the commit comment of the `develop` branch of the bionic porting main repository, <https://github.com/aosp-riscv/port_bionic>, and the commit comments of the `develop` branch of its submodule repositories. The next step will be to continue to improve the libc of bionic and the self-made minimal root file system.

- 2020-10-30 The make framework has been improved to support the compilation and generation of libc static libraries, as well as `crtbegin.o` and `crtend.o`. Currently, the statically compiled executable program can jump from the kernel to the main function and ensure that `argv` and `envp` are read correctly. The porting of other TLS and syscall parts is still ongoing. For specific changes, please refer to the `develop` branch of <https://github.com/aosp-riscv/port_bionic>.

- 2020-10-16 Simplify the build framework of the AOSP bionic library to make, and submit the preliminary porting work to the develop branch of <https://github.com/aosp-riscv/port_bionic>. Currently, the project repository depends on two sub-repositories: <https://github.com/aosp-riscv/platform_bionic> and <https://github.com/aosp-riscv/external_jemalloc_new> (Because the changes are still drafts, the current changes to AOSP are all placed on their respective `develop` branches. Another note: the current porting work is based off `android-10.0.0_r39` tag version based on AOSP). The next step is to continue to improve the make project and make changes to the bionic code. The first goal is to implement `libc.a`.

- 2020-10-01 Using the latest AOSP kernel and configuration, with GNU and LLVM/Clang tool-chain for RISC-V, we successfully compiled and booted the kernel on QEMU. The only problem is: after mounting the minimum rootfs, we encounter a "Segmentation Fault" error. Still debugging this issue. In addition, we began to study how to port the bionic library of AOSP to RISC-V. The current work is to port the AOSP building framework for bionic.

- 2020-09-15 We launched the task to port the AOSP kernel to RISC-V. After some research, we decided to base it off the branch `android-5.4-stable` of AOSP common kernels and we are working on it now.

- 2020-09-01 We fixed the errors in build dir and prebuilts dir when using the 'mmm art/compiler' to generate the ninja files for the compiler module. We are debugging the errors in the system dir and cts dir.

- 2020-08-15 We have added the lunch menu for RISC-V, and successfully loaded the menu. After that, we added the RISC-V config files for build/make and build/soong, We are debugging the RISC-V config files in build/soong.

## About Us

The initial members of this project are from the [PLCT lab](https://github.com/isrc-cas/).
Wei Wu (@lazyparser) and Ningning Shi (@shining1984) co-founded this project.

We would like to contribute all of the efforts to the RISC-V community and the AOSP project.

## Join Us

If you want to contribute, please read the contribution guide on the AOSP website.

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
- [**First "Android minimal system" for RISC-V, Wang Chen - PLCT lab, 20201120 (Chinese Version)**](https://zhuanlan.zhihu.com/p/302870095)
- [**Create a minimal Android system for RISC-V, Wang Chen - PLCT lab, 20201124**](https://plctlab.github.io/aosp/create-a-minimal-android-system-for-riscv.html)
- [**RISC-V Gets an Early, Minimal Android 10 Port Courtesy of PLCT Lab, Gareth Halfacree - https://abopen.com/, 20201127**](https://abopen.com/news/risc-v-gets-an-early-minimal-android-10-port-courtesy-of-plct-lab/)
- [**AOSP-RISCV has a new mirror on Gitee, Wang Chen - PLCT lab, 20201215 (Chinese Version)**](https://zhuanlan.zhihu.com/p/337032693)
- [**Summary of related knowledge behind AOSP build, Wang Chen - PLCT lab, 20201230**](https://zhuanlan.zhihu.com/p/340689022)
- [**Details about AOSP Soong creation process, Wang Chen - PLCT lab, 20210108**](https://zhuanlan.zhihu.com/p/342817768)

### Videos

- [**[COSCUP 2011] Porting android to brand-new CPU architecture, Luse Cheng (Traditional Chinese)**](https://www.youtube.com/watch?v=li6PqLn4Bl4)
- [**How difficult when do RISC-V porting for AOSP - Wu Wei - V8 technical symposium - OSDT community - 20200607 (Chinese Version)**](https://www.bilibili.com/video/BV1wC4y1a7Za)
- [**Introduction about Building framwork of AOSP and preliminary trying porting for RISC-V - Wang Chen - 20200805 - PLCT lab (Chinese Version)**](https://www.bilibili.com/video/BV1PA411Y7mz)
- [**AOSP for RISC-V porting tutorial (1) - Introduction about Android Runtime - Wang Chen - 20200814 - PLCT lab (Chinese Version)**](https://www.bilibili.com/video/BV1wC4y1t7Xa)
- [**AOSP for RISC-V porting tutorial (2) - Starting porting ART - Wang Chen - 20200821 - PLCT lab (Chinese Version)**](https://www.bilibili.com/video/BV1JK411M7e5)

### Discussions

- [**The status of AOSP porting: Is there anyone working on it (publicly)?**](https://groups.google.com/a/groups.riscv.org/g/sw-dev/c/u9iP7A2Wkc8)

### Other Resources

- [**Android-x86**](https://www.android-x86.org/)

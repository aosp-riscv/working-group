# AOSP on RISC-V 工作组

本仓库适用于为 AOSP for RISC-V 项目组归档除代码外的其他相关资料和讯息。

如果您对在基于 RISC-V 架构的硬件上运行 Android，欢迎加入我们！

## 项目状态

本项目还处于非常早期的状态，更多的工作在持续推进中。

- 2020-08-15 尝试修改 AOSP 的构造框架，在 lunch 菜单中加入 RISC-V 设备条目。我们还为 `build/make` 和 `build/soong` 增加了相关的配置项，目前还在调试 `build/soong` 中的问题。

- 2020-09-01 修复了运行 `mmm art/compiler` 命令在生成 ninja 文件过程中的编译错误，主要涉及 build 和 prebuilts 仓库下修改。我们目前调试的主要问题集中在 system 和 cts 仓库中。

- 2020-09-15 开始为 RISC-V 移植 AOSP 的内核，通过调研基本确定基于 AOSP common kernel 的 `android-5.4-stable` 分支开展移植工作，具体移植工作正在进行中。

- 2020-10-01 采用最新的 AOSP 内核及配置，尝试使用 RISC-V 的 GNU 工具链和 LLVM/Clang 工具链均可以成功编译并启动，但在挂载最小根文件系统后遇到 “Segmentation Fault” 错误，继续调试中。另外开始研究如何将 AOSP 的 bionic 库移植到 RISC-V 上，目前主要工作是在修改移植编译框架。

- 2020-10-16 将 AOSP bionic 库的 build 框架简化为 make，初步的移植工作提交到 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支，目前该项目仓库依赖于两个子仓库 <https://github.com/aosp-riscv/platform_bionic> 和 <https://github.com/aosp-riscv/external_jemalloc_new> （由于改动还很 draft，所以目前对 aosp 相关的改动全部放在各自的 develop 分支上。另备注：目前的移植工作基于 AOSP 的 `android-10.0.0_r39` tag 版本）。下一步的工作是继续完善 make 项目并对 bionic 的代码进行移植改动，第一个目标是实现 `libc.a`。

- 2020-10-30 完善了 make 框架，支持编译生成 libc 静态库以及 `crtbegin.o` 和 `crtend.o`。目前可以实现静态编译的可执行程序从内核跳转进入 main 函数并确保 argv 和 envp 读取正确，其他 TLS 和 syscall 部分的移植还在进行中。具体改动请参考 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支。

- 2020-11-13 进一步完善 bionic 的 libc 静态库。目前采用 v10.0.1 的 llvm/clang 进行编译，然后用 gnu riscv 版本的 ld 基于 bionic 的 libc_static 、crtbegin_static 和 crtend.o 可以实现静态链接，并生成可执行程序。在此基础上移植编译了 AOSP 自带的 toybox 并制作了一个最小的 rootfs（shell 和 init 还是采用基于 glibc 编译的 busybox）。目前该 rootfs 可以在 qemu-system-riscv64 上启动运行。可以运行部分命令，但还有部分命令执行会报错。具体改动请参考 bionic 移植主仓库 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支以及其 submodule 仓库的 develop 分支的 commit comment。下一步将继续完善 bionic 的 libc，以及完善自制的最小根文件系统。

- 2020-11-30 完成 shell 和 init 的移植，目前可以实现完全基于 bionic 的 libc 库，采用静态链接的方式生成一个最小的 android 根文件系统。该 rootfs 可以在 qemu-system-riscv64 上启动运行，并支持运行一些基本的操作命令。此外还改进了原先的 make 框架系统，为下一个阶段进一步支持动态链接做准备。具体改动请参考 bionic 移植主仓库 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支以及其相应的 submodule 仓库。更多的详细介绍可以参考另一篇知乎介绍文章：[《第一个 RISC-V 上的“Android 最小系统”》](https://zhuanlan.zhihu.com/p/302870095)。下一步的工作重点是增加动态链接支持，继续完善 bionic 功能并尝试移植 AOSP 的 Soong 构造系统，支持采用 AOSP 的 Soong 构造系统编译 RISC-V 版本的 bionic 库和相关应用。

- 2020-12-15 
    - 开始移植 bionic 的动态链接功能，目前完成了 `libc.so` 和 `linker` 的编译链接，但运行还有问题，还在调试中，欢迎熟悉动态链接器实现或者对此感兴趣的小伙伴一起来研究。此外还进一步改进优化了 make 框架。具体改动请参考 bionic 移植主仓库 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支以及其相应的 submodule 仓库。
    - 目前移植工作涉及的 AOSP 子仓库已经达到 9 个，全部下载完有 537M。为了方便中国国内的小伙伴下载访问，我们在 Gitee 上为 <https://github.com/aosp-riscv> 建了一个 mirror 网站，具体地址是 <https://gitee.com/aosp-riscv/>。具体访问操作说明请参考另外一篇知乎介绍文章 [《AOSP-RISCV 的开源仓库在 Gitee 上新建了镜像》](https://zhuanlan.zhihu.com/p/337032693)。
    - 下一步的工作重点依然是实现动态链接支持，目前感觉实现难度比静态链接要大。除此之外会继续完善 bionic 功能并尝试移植 AOSP 的 Soong 构造系统，支持采用 AOSP 的 Soong 构造系统编译 RISC-V 版本的 bionic 库和相关应用。

## 有关我们

项目初创人员来自 [PLCT lab](https://github.com/isrc-cas/).
吴伟 (@lazyparser) 和 史宁宁 (@shining1984) 共同创建了本项目。

非常高兴能够为 RISC-V 社区和 AOSP 项目贡献我们的力量。

## 加入我们

如果您对本项目感兴趣，请阅读 AOSP 网站的相关指导文件。

如果您有话题想进一步参与讨论，请在 github 上给我们提交 issue。

## 相关资料

### 幻灯

- [**Porting Android to new Platforms, Amit Pundir**](https://www.slideshare.net/linaroorg/porting-android-tonewplatforms)

### 文章

- [**AOSP 的版本管理，汪辰（ PLCT 实验室），20200911**](https://zhuanlan.zhihu.com/p/234390474)
- [**AOSP 内核的版本管理，汪辰（ PLCT 实验室），20200915**](https://zhuanlan.zhihu.com/p/245131105)
- [**在 QEMU 上运行 RISC-V 64 位版本的 Linux，汪辰（ PLCT 实验室），20200923**](https://zhuanlan.zhihu.com/p/258394849)
- [**编译一个 RISC-V 的 Android 内核，汪辰（ PLCT 实验室），20200929**](https://zhuanlan.zhihu.com/p/260356339)
- [**制作一个针对 RISC-V 的 LLVM/Clang 编译器，汪辰（ PLCT 实验室），20201009**](https://zhuanlan.zhihu.com/p/263550372)
- [**第一个 RISC-V 上的 “Android 最小系统”，汪辰（ PLCT 实验室），20201120**](https://zhuanlan.zhihu.com/p/302870095)
- [**Create a minimal Android system for RISC-V, Wang Chen - PLCT lab, 20201124**](https://plctlab.github.io/aosp/create-a-minimal-android-system-for-riscv.html)
- [**RISC-V Gets an Early, Minimal Android 10 Port Courtesy of PLCT Lab, Gareth Halfacree - https://abopen.com/, 20201127**](https://abopen.com/news/risc-v-gets-an-early-minimal-android-10-port-courtesy-of-plct-lab/)
- [**AOSP-RISCV 的开源仓库在 Gitee 上新建了镜像，汪辰（ PLCT 实验室），20201215**](https://zhuanlan.zhihu.com/p/337032693)

### 视频

- [**[COSCUP 2011] Porting android to brand-new CPU architecture, 鄭孟璿 (Luse Cheng)**](https://www.youtube.com/watch?v=li6PqLn4Bl4)
- [**闪电：AOSP 移植 RISC-V 有多难 - 吴伟 - V8 技术讨论会 - OSDT 社区 - 20200607**](https://www.bilibili.com/video/BV1wC4y1a7Za)
- [**AOSP 的构建系统和 RISC-V 移植初步 - 汪辰 - 20200805 - PLCT 实验室**](https://www.bilibili.com/video/BV1PA411Y7mz)
- [**AOSP for RISC-V 移植教程之 Android Runtime 介绍 - 汪辰 - 20200814 - PLCT 实验室**](https://www.bilibili.com/video/BV1wC4y1t7Xa)
- [**AOSP for RISC-V 移植教程之 开始 ART 移植 - 汪辰 - 20200821 - PLCT 实验室**](https://www.bilibili.com/video/BV1JK411M7e5)

### 讨论

- [**The status of AOSP porting: Is there anyone working on it (publicly)?**](https://groups.google.com/a/groups.riscv.org/g/sw-dev/c/u9iP7A2Wkc8)

### 其他资源

- [**Android-x86**](https://www.android-x86.org/)

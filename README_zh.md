# AOSP on RISC-V 工作组

本仓库适用于为 AOSP for RISC-V 项目组归档除代码外的其他相关资料和讯息。

如果您对在基于 RISC-V 架构的硬件上运行 Android 感兴趣，欢迎加入我们！

AOSP-RISCV 的代码开源，欢迎大家参与贡献。所有仓库在 Github 和 Gitee 上一式两份，互为镜像备份。

- Github 仓库地址：<https://github.com/aosp-riscv>
- Gitee 仓库地址： <https://gitee.com/aosp-riscv>

大家可以根据自己的喜好选择代码仓库进行开发，两边采用类似的开发流程：

- [基于 Github 开发的流程说明](./docs/dev-github.md)
- [基于 Gitee 开发的流程说明](./docs/zh/dev-gitee.md)

## 项目状态

本项目还处于非常早期的状态，更多的工作在持续推进中。以下为项目进展日志（时间为倒序）。

- 2022-01-20 状态更新
  - 运行 bionic 单元测试并解决发现的问题，提交的 PR 列表如下：
    - [fix TLS issues](https://gitee.com/aosp-riscv/platform_bionic/pulls/5)
    - [change TLS slot organization same as that for arm](https://gitee.com/aosp-riscv/platform_bionic/pulls/6)
    - [fixed vfork](https://gitee.com/aosp-riscv/platform_bionic/pulls/7)
    - [continue vfork](https://gitee.com/aosp-riscv/platform_bionic/pulls/8)
    - [fixed memset infinite loop issue](https://gitee.com/aosp-riscv/platform_bionic/pulls/10)
    - [some changes for test env](https://gitee.com/aosp-riscv/test-riscv/pulls/3)
    - [run test with isolate mode](https://gitee.com/aosp-riscv/test-riscv/pulls/4)
    - [enabled some blocking cases](https://gitee.com/aosp-riscv/test-riscv/pulls/5)
    - [minor changes and ready for upstream](https://gitee.com/aosp-riscv/platform_bionic/pulls/9)
    - [code format improvement](https://gitee.com/aosp-riscv/platform_bionic/pulls/11)
    - [minor fix and ready for upstream](https://gitee.com/aosp-riscv/platform_build/pulls/2)
    - [minor fix and ready for upstream](https://gitee.com/aosp-riscv/platform_build_soong/pulls/3)
  - 文章和技术手册更新：
    - [continue update android kernel related knowledge](https://gitee.com/aosp-riscv/working-group/pulls/10)
    - [updated article: platform-version](https://gitee.com/aosp-riscv/working-group/pulls/11)
    - [add how to build clang for aosp](https://gitee.com/aosp-riscv/working-group/pulls/12)
  - RVI aosp 12 上游提交工作
    - [RVI Android SIG 发布 AOSP 12 源码仓库, Jan/17/2022](https://lists.riscv.org/g/sig-android/message/32), 
    - [removed old seccomp txt files](https://github.com/riscv-android-src/platform-bionic/pull/3)
    - [bugfix: enable rela ifunc resolver for riscv64](https://github.com/riscv-android-src/platform-bionic/pull/4)
    - [format code style](https://github.com/riscv-android-src/platform-bionic/pull/6)

- 2021-12-23 状态更新
  - 搭建了一个最小系统，在 QEMU 上运行 AOSP 的 bionic-unit-test-static，并解决测试中发现的 bugs。
    - malloc.malloc_info: SIGABRT：<https://gitee.com/aosp-riscv/test-riscv/pulls/2>
    - part of stdio cases FAILED due to can not create tmpfiles: <https://gitee.com/aosp-riscv/test-riscv/pulls/2>
    - Fixed wrong mdev path issue: <https://gitee.com/aosp-riscv/test-riscv/pulls/2>
    - ifunc.*：Segmentation fault: <https://gitee.com/aosp-riscv/platform_bionic/pulls/4>; 该 bugfix 也提交上游 PR: <https://github.com/riscv-android-src/platform-bionic/pull/1>
    - added bionic-unit-tests-static log: <https://gitee.com/aosp-riscv/test-riscv/pulls/2>
    - Added doc on how-to setup test env: <https://gitee.com/aosp-riscv/test-riscv/pulls/2>
  - 参加 OSDTConf2021 并做 AOSP for RISC-V 社区开源进展报告
    - 报告 slides: <https://github.com/plctlab/PLCT-Open-Reports/blob/master/20211218-osdt2021-aosp-rv-wangchen.pdf>
    - 报告 vedio: <https://www.bilibili.com/video/BV1Sg411w7Le>

- 2021-12-10 已可以完整地成功编译 bionic，正在利用 Google 提供的 gtest 套件运行集成测试。
  - Clang 工具链中增加 libFuzzer 支持 riscv
    - <https://github.com/aosp-riscv/toolchain_llvm_android/pull/1>
    - <https://github.com/aosp-riscv/toolchain_llvm-project/pull/1>
    - <https://github.com/aosp-riscv/platform-prebuilts-clang-host-linux-x86/commit/5d06484b069ec0af9f0d278a3fcc2559bb6f37f4>
  - 使能 soong tests，构建中无需输入 `--skip-soong-tests`
    - <https://github.com/aosp-riscv/platform_build_soong/pull/1>
    - <https://github.com/aosp-riscv/platform-build-bazel/pull/1>
  - 针对一些大仓库，不再使用 patch 方式，仍然使用 git 进行管理:
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/10>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-runtime/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-packages-services-Car/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-packages-modules-ArtPrebuilt/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-frameworks-base/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-cts/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-clang-host-linux-x86/pulls/1>
  - 构建成功 bionic，使用 "mmm bionic":
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/7>
    - <https://gitee.com/aosp-riscv/platform-external-llvm/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_bionic/pulls/3>
    - <https://gitee.com/aosp-riscv/platform-external-scudo/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-unwinding/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-libbase/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_build_soong/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/8>
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/9>
  - 增加一个仓库存放测试相关的脚本: 
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/11>

- 2021-11-30 针对 AOSP 12 移植到 RV64 实现 `mmm bionic/libc/ --skip-soong-tests` 下构建成功 libc/libm/libdl。相关修改已经提交到仓库上。具体如下(部分 PR 在 gitee 上，所有代码同步推送至 github 和 gitee)：
  - 工具链 for AOSP 更新：
    - Rust for Android:
      - compiler: add riscv64gc-linux-android target (Tier 3): <https://github.com/aosp-riscv/rust/commit/8397a76b895b586dcfc8637e98908f65df1af1f9>
      - library: add riscv64gc-linux-android support: <https://github.com/aosp-riscv/rust/commit/2eacf985415ba38018de70845c0b150885ad4d57>
      - Modified libc, added support for riscv64：<https://github.com/aosp-riscv/toolchain_rustc/pull/1>
    - Clang/llvm for Android:
      - add riscv __get_tls method: <https://github.com/aosp-riscv/toolchain_llvm-project/commit/2511f435f0c0c02f1a64d55b3b45c657dc84b050>
      - Change some repos' fetch address and revision to support riscv64: <https://github.com/aosp-riscv/platform_manifest/commit/a3446a1580947ac5d5d29e99365fdf90b0edb3ef>
  - manifest 仓库：PR: <https://gitee.com/aosp-riscv/platform_manifest/pulls/7>
  - AOSP 仓库更新（PR）：
    - <https://gitee.com/aosp-riscv/platform-external-scudo/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-unwinding/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-libbase/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_build_soong/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_bionic/pulls/2>
    - <https://gitee.com/aosp-riscv/platform-external-kernel-headers/pulls/1>

- 2021-11-15 针对 AOSP 12 移植到 RV64 实现 `m --skip-ninja --skip-soong-tests` 下构建成功。相关修改已经提交到仓库上。具体如下(这里以 gitee 的为例，github 已经 mirror)：
  - manifest 仓库：<https://gitee.com/aosp-riscv/platform_manifest>
  - 涉及新增的 AOSP 仓库：
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
  - 另有部分仓库由于整体体积过大或者部分文件体积过大，超过 gitee/github 限制的，目前无法在 gitee/github 上新建仓库，所以目前采用补丁方式保存修改、目前这些涉及的仓库为：
    - cts/
    - frameworks/base/
    - packages/modules/ArtPrebuilt/
    - packages/services/Car/
    - prebuilts/clang/host/linux-x86/
    - prebuilts/runtime/
    新建的补丁仓库为: <https://gitee.com/aosp-riscv/patches_aosp_riscv>
  - 提交的 PR 列表如下:
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
  - 更新文档 [“如何编译 aosp-riscv” ](./articles/20211029-howto-setup-build-env.md)
  - 新增文档 ["代码走读：对 soong_ui 的深入理解"](./articles/20211102-codeanalysis-soong_ui.md)

- 2021-10-29 重启 PLCT lab 的 AOSP 移植工作，最新的目标是将 AOSP 12 移植到 RV64 上。
  - 创建 group @ <https://github.com/aosp-riscv> (mirrored with <https://gitee.com/aosp-riscv>) 以及部分仓库.
  - [PR Merged] added note on howto setup build env: <https://gitee.com/aosp-riscv/working-group/pulls/1>
  - 新增文档 ["Code analysis for envsetup"](./articles/20211026-lunch.md)
  - 新增文档 ["Howto setup build environment"](./articles/20211029-howto-setup-build-env.md)

- 2021-01-xx 由于平头哥的 [开源工作 ](https://github.com/T-head-Semi/aosp-riscv)，我们于 2021 年初停止了 PLCT lab 的相关 AOSP 移植工作。所有原 <https://github.com/aosp-riscv> 和 <https://gitee.com/aosp-riscv> 下的代码仓库（除了 working-group）都备份到 [Gitee 的 aosp-riscv-bionic-porting 组织](https://gitee.com/aosp-riscv-bionic-porting) 下。

- 2021-01-15 仍然在研究如何基于 AOSP 的 Soong 框架加入 RVG64。目前的主要难点是由于 AOSP 的编译系统异常庞大和复杂，包含了太多工程化所需的内容，而且互相交织和依赖在一起，所以如何干净地先屏蔽掉我们不关心的模块，只编译 bionic 库的核心内容是目前的重点也是难点。目前还没有太多的头绪，如果有好心的小伙伴欢迎来提供帮助和建议。

- 2020-12-29 完成 bionic 的动态链接功能，目前可以基于 bionic 支持实现隐式动态链接和显式动态链接。此外还改进了原先的 make 框架系统。具体改动请参考 bionic 移植主仓库 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支以及其相应的 submodule 仓库（国内用户也可以访问 Gitee 镜像 <https://gitee.com/aosp-riscv/>）。下一步的工作重点是移植修改 AOSP 的 Soong 构造系统，支持采用 AOSP 提供的 Soong 构造系统编译 RISC-V 版本的 bionic 库和相关应用。

- 2020-12-15
    - 开始移植 bionic 的动态链接功能，目前完成了 `libc.so` 和 `linker` 的编译链接，但运行还有问题，还在调试中，欢迎熟悉动态链接器实现或者对此感兴趣的小伙伴一起来研究。此外还进一步改进优化了 make 框架。具体改动请参考 bionic 移植主仓库 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支以及其相应的 submodule 仓库。
    - 目前移植工作涉及的 AOSP 子仓库已经达到 9 个，全部下载完有 537M。为了方便中国国内的小伙伴下载访问，我们在 Gitee 上为 <https://github.com/aosp-riscv> 建了一个 mirror 网站，具体地址是 <https://gitee.com/aosp-riscv/>。具体访问操作说明请参考另外一篇知乎介绍文章 [《AOSP-RISCV 的开源仓库在 Gitee 上新建了镜像》](https://zhuanlan.zhihu.com/p/337032693)。
    - 下一步的工作重点依然是实现动态链接支持，目前感觉实现难度比静态链接要大。除此之外会继续完善 bionic 功能并尝试移植 AOSP 的 Soong 构造系统，支持采用 AOSP 的 Soong 构造系统编译 RISC-V 版本的 bionic 库和相关应用。

- 2020-11-30 完成 shell 和 init 的移植，目前可以实现完全基于 bionic 的 libc 库，采用静态链接的方式生成一个最小的 android 根文件系统。该 rootfs 可以在 qemu-system-riscv64 上启动运行，并支持运行一些基本的操作命令。此外还改进了原先的 make 框架系统，为下一个阶段进一步支持动态链接做准备。具体改动请参考 bionic 移植主仓库 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支以及其相应的 submodule 仓库。更多的详细介绍可以参考另一篇知乎介绍文章：[《第一个 RISC-V 上的“Android 最小系统”》](https://zhuanlan.zhihu.com/p/302870095)。下一步的工作重点是增加动态链接支持，继续完善 bionic 功能并尝试移植 AOSP 的 Soong 构造系统，支持采用 AOSP 的 Soong 构造系统编译 RISC-V 版本的 bionic 库和相关应用。

- 2020-11-13 进一步完善 bionic 的 libc 静态库。目前采用 v10.0.1 的 llvm/clang 进行编译，然后用 gnu riscv 版本的 ld 基于 bionic 的 libc_static 、crtbegin_static 和 crtend.o 可以实现静态链接，并生成可执行程序。在此基础上移植编译了 AOSP 自带的 toybox 并制作了一个最小的 rootfs（shell 和 init 还是采用基于 glibc 编译的 busybox）。目前该 rootfs 可以在 qemu-system-riscv64 上启动运行。可以运行部分命令，但还有部分命令执行会报错。具体改动请参考 bionic 移植主仓库 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支以及其 submodule 仓库的 develop 分支的 commit comment。下一步将继续完善 bionic 的 libc，以及完善自制的最小根文件系统。

- 2020-10-30 完善了 make 框架，支持编译生成 libc 静态库以及 `crtbegin.o` 和 `crtend.o`。目前可以实现静态编译的可执行程序从内核跳转进入 main 函数并确保 argv 和 envp 读取正确，其他 TLS 和 syscall 部分的移植还在进行中。具体改动请参考 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支。

- 2020-10-16 将 AOSP bionic 库的 build 框架简化为 make，初步的移植工作提交到 <https://github.com/aosp-riscv/port_bionic> 的 develop 分支，目前该项目仓库依赖于两个子仓库 <https://github.com/aosp-riscv/platform_bionic> 和 <https://github.com/aosp-riscv/external_jemalloc_new> （由于改动还很 draft，所以目前对 aosp 相关的改动全部放在各自的 develop 分支上。另备注：目前的移植工作基于 AOSP 的 `android-10.0.0_r39` tag 版本）。下一步的工作是继续完善 make 项目并对 bionic 的代码进行移植改动，第一个目标是实现 `libc.a`。

- 2020-10-01 采用最新的 AOSP 内核及配置，尝试使用 RISC-V 的 GNU 工具链和 LLVM/Clang 工具链均可以成功编译并启动，但在挂载最小根文件系统后遇到 “Segmentation Fault” 错误，继续调试中。另外开始研究如何将 AOSP 的 bionic 库移植到 RISC-V 上，目前主要工作是在修改移植编译框架。

- 2020-09-15 开始为 RISC-V 移植 AOSP 的内核，通过调研基本确定基于 AOSP common kernel 的 `android-5.4-stable` 分支开展移植工作，具体移植工作正在进行中。

- 2020-09-01 修复了运行 `mmm art/compiler` 命令在生成 ninja 文件过程中的编译错误，主要涉及 build 和 prebuilts 仓库下修改。我们目前调试的主要问题集中在 system 和 cts 仓库中。

- 2020-08-15 尝试修改 AOSP 的构造框架，在 lunch 菜单中加入 RISC-V 设备条目。我们还为 `build/make` 和 `build/soong` 增加了相关的配置项，目前还在调试 `build/soong` 中的问题。

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
- [**AOSP Build 背后涉及的相关知识汇总，汪辰（ PLCT 实验室），20201230**](https://zhuanlan.zhihu.com/p/340689022)
- [**AOSP Soong 创建过程详解，汪辰（ PLCT 实验室），20210108**](https://zhuanlan.zhihu.com/p/342817768)

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


本周期（2022/10/28 ~ 2022/11/10）RISCV 相关 merge PR 汇总参考 [这里][1]。

Google 于 10 月 1 日宣布正式开始对 AOSP 项目接收 RISC-V 的提交 PR，所以我们在 <https://android-review.googlesource.com/> 开始看到相关的修改。

本周期的修改总结主要集中在下面几个地方：

<!-- TOC -->

- [1. Build System](#1-build-system)
	- [1.1. platform/build](#11-platformbuild)
	- [1.2. platform/soong](#12-platformsoong)
	- [1.3. platform/bazel](#13-platformbazel)
	- [1.4. platform/manifest](#14-platformmanifest)
- [2. Bionic](#2-bionic)
	- [2.1. 内核头文件处理](#21-内核头文件处理)
	- [2.2. libc 导出符号处理](#22-libc-导出符号处理)
	- [2.3. libc 的 API 处理](#23-libc-的-api-处理)
	- [2.4. libc 中的 TLS 支持](#24-libc-中的-tls-支持)
	- [2.5. libm](#25-libm)
	- [2.6. linker](#26-linker)
	- [2.7. 其他未分类：](#27-其他未分类)
- [3. 内核（linux）](#3-内核linux)
	- [3.1. kernel/tests](#31-kerneltests)
- [4. Toolchain](#4-toolchain)
	- [4.1. toolchain/llvm_android](#41-toolchainllvm_android)
	- [4.2. toolchain/llvm-project](#42-toolchainllvm-project)
	- [4.3. toolchain/rustc](#43-toolchainrustc)
	- [4.4. platform/prebuilts/clang/host/linux-x86/](#44-platformprebuiltsclanghostlinux-x86)
- [5. System](#5-system)
	- [5.1. platform/system/core](#51-platformsystemcore)
	- [5.2. 其他](#52-其他)
- [6. Framework](#6-framework)
	- [6.1. platform/art](#61-platformart)
	- [6.2. 其他未整理](#62-其他未整理)
- [7. 模拟器部分](#7-模拟器部分)
- [8. 未归类的其他](#8-未归类的其他)

<!-- /TOC -->

# 1. Build System

## 1.1. platform/build

具体涉及 PR 包括：

详细说明：

## 1.2. platform/soong

具体涉及 PR 包括：

详细说明：

## 1.3. platform/bazel

具体涉及 PR 包括：

详细说明：

## 1.4. platform/manifest

具体涉及 PR 包括：

详细说明：

# 2. Bionic

Bionic 库的修改是目前 RVI Android SIG 牵头提交的大头，原始提交参考这里 [[RFC]Add riscv64 support][3]，但由于改动较大，而且目前 [RVI 维护的仓库][6] 还是基于 AOSP 12 的，所以 Google 团队打算将其分成更小的子补丁分批合入主线，具体讨论可以参考 [here][4] 和 [there][5]。

## 2.1. 内核头文件处理

这些修改都和更新 bionic 的 libc 依赖的内核头文件有关，bionic 的 libc 提供的 c lib 头文件会引用内核的头文件，类似 glibc。

具体涉及 PR 包括：

详细说明：

## 2.2. libc 导出符号处理

这些修改都和 bionic 支持的 symbol version 机制有关，有关 symbol versioning 的概念可以参考 [《学习笔记: Symbol Versioning 基本使用》][2]。

具体涉及 PR 包括：

详细说明：

## 2.3. libc 的 API 处理

这些修改都是针对 libc 的 API （POSIX）部分增加 riscv 的分支处理。

具体涉及 PR 包括：

详细说明：

## 2.4. libc 中的 TLS 支持

这些都是和 bionic 中支持 TLS（Thread Local Storage） 有关。

具体涉及 PR 包括：

详细说明:

## 2.5. libm

boinic 中的数学库。

具体涉及 PR 包括：

详细说明:

## 2.6. linker

boinic 中的动态链接器。

具体涉及 PR 包括：

详细说明:

## 2.7. 其他未分类：

具体涉及 PR 包括：

详细说明:

# 3. 内核（linux）

具体涉及 PR 包括：

详细说明:

## 3.1. kernel/tests

具体涉及 PR 包括：

详细说明:

# 4. Toolchain

## 4.1. toolchain/llvm_android

toolchain/llvm_android 是有关 llvm/clang 的构建脚本仓库。

具体涉及 PR 包括：

详细说明：

## 4.2. toolchain/llvm-project

llvm/clang 的官方仓库在 google 这里的 mirror 以及包含 google 的补丁。

具体涉及 PR 包括：

详细说明:

## 4.3. toolchain/rustc

rustc 仓库

具体涉及 PR 包括：

详细说明:

## 4.4. platform/prebuilts/clang/host/linux-x86/

具体涉及 PR 包括：

详细说明:

# 5. System

AOSP 的 system image 的核心部分

## 5.1. platform/system/core

具体涉及 PR 包括：

详细说明：

## 5.2. 其他

具体涉及 PR 包括：

详细说明:

# 6. Framework

## 6.1. platform/art

ART 的仓库

具体涉及 PR 包括：

详细说明:

## 6.2. 其他未整理

具体涉及 PR 包括：

详细说明：

# 7. 模拟器部分

具体涉及 PR 包括：

详细说明：


# 8. 未归类的其他

具体涉及 PR 包括：

详细说明:

[1]: https://unicornx.github.io/android-review/aosp-riscv-2022-10-28.html
[2]: ../20221008-symbol-version.md
[3]: https://android-review.googlesource.com/c/platform/bionic/+/2142912
[4]: https://android-review.googlesource.com/c/platform/bionic/+/2142912/1/libc/arch-riscv64/bionic/__bionic_clone.S
[5]: https://android-review.googlesource.com/c/platform/bionic/+/2241712/comment/b3dfabdf_bdbd33ef/
[6]: https://gitee.com/aosp-riscv/working-group/issues/I5BV63
[7]: https://gitee.com/aosp-riscv/working-group/issues/I5CKA4

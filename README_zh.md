# AOSP on RISC-V 工作组

本仓库适用于为 AOSP for RISC-V 项目组归档除代码外的其他相关资料和讯息。

如果您对在基于 RISC-V 架构的硬件上运行 Android，欢迎加入我们！

## 项目状态

本项目还处于非常早期的状态，更多的工作在持续推进中。

- 2020-08-15 尝试修改 AOSP 的构造框架，在 lunch 菜单中加入 RISC-V 设备条目。我们还为 `build/make` 和 `build/soong` 增加了相关的配置项，目前还在调试 `build/soong` 中的问题。

- 2020-09-01 修复了运行 `mmm art/compiler` 命令在生成 ninja 文件过程中的编译错误，主要涉及 build 和 prebuilts 仓库下修改。我们目前调试的主要问题集中在 system 和 cts 仓库中。

- 2020-09-15 开始为 RISC-V 移植 AOSP 的内核，通过调研基本确定基于 AOSP common kernel 的 `android-5.4-stable` 分支开展移植工作，具体移植工作正在进行中。

- 2020-10-01 采用最新的 AOSP 内核及配置，尝试使用 RISC-V 的 GNU 工具链和 LLVM/Clang 工具链均可以成功编译并启动，但在挂载最小根文件系统后遇到 “Segmentation Fault” 错误，继续调试中。另外开始研究如何将 AOSP 的 bionic 库移植到 RISC-V 上，目前主要工作是在修改移植编译框架。

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

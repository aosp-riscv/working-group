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

- [**AOSP 的版本管理，汪辰（ PLCT 实验室），20200911**](https://zhuanlan.zhihu.com/p/234390474)
- [**AOSP 内核的版本管理，汪辰（ PLCT 实验室），20200915**](https://zhuanlan.zhihu.com/p/245131105)


### Vedios

- [**[COSCUP 2011] Porting android to brand-new CPU architecture, 鄭孟璿 (Luse Cheng)**](https://www.youtube.com/watch?v=li6PqLn4Bl4)

- [**闪电：AOSP 移植 RISC-V 有多难 - 吴伟 - V8 技术讨论会 - OSDT 社区 - 20200607**](https://www.bilibili.com/video/BV1wC4y1a7Za)

- [**AOSP 的构建系统和 RISC-V 移植初步 - 汪辰 - 20200805 - PLCT 实验室**](https://www.bilibili.com/video/BV1PA411Y7mz)

- [**AOSP for RISC-V 移植教程之 Android Runtime 介绍 - 汪辰 - 20200814 - PLCT实 验室**](https://www.bilibili.com/video/BV1wC4y1t7Xa)

- [**AOSP for RISC-V 移植教程之 开始 ART 移植 - 汪辰 - 20200821 - PLCT 实验室**](https://www.bilibili.com/video/BV1JK411M7e5)

### Discussions

- [**The status of AOSP porting: Is there anyone working on it (publicly)?**](https://groups.google.com/a/groups.riscv.org/g/sw-dev/c/u9iP7A2Wkc8)

### Other Resources 

- [**Android-x86**](https://www.android-x86.org/)

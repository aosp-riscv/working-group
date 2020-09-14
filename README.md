# AOSP on RISC-V WG

THis is meta repo for AOSP for RISC-V Project. General tasks and issues here.

If you are interested in runnning Android on RISC-V hardware, please join us!

## Status

Currently we are in a very early stage, trying to cross compile AOSP codebase
using the RISC-V official GNU toolchain.

- 2020-08-15 We have added the lunch menu for RISC-V, and loaded the menu success.
After that, we added the RISC-V config files for build/make and build/soong,
we are debugging RISC-V the config files in build/soong.

- 20200901 We fixed the errors in build dir and prebuilts dir when we using the
'mmm art/compiler' to generate the ninja files for compiler module. We are debugging
the errors in the system dir and cts dir.

## About Us

The initial members of this project is from the [PLCT lab](https://github.com/isrc-cas/).
Wei Wu (@lazyparser) and Ningning Shi (@shining1984) co-found this project.

We would like to contribute all the efforts to RISC-V community and AOSP project.

## Join Us

If you want to contribute, please read the contributing guide from the AOSP website.

Feel free to open an issue for further questions/discussions!

## Learning Materials

### English


**Porting Android to new Platforms**

Amit Pundir

https://www.slideshare.net/linaroorg/porting-android-tonewplatforms


**Android-x86**

https://www.android-x86.org/

### Traditional Chinese

**[COSCUP 2011] Porting android to brand-new CPU architecture**

鄭孟璿 (Luse Cheng)

https://www.youtube.com/watch?v=li6PqLn4Bl4

### Chinese Simplified

**AOSP的构建系统和RISC-V移植初步 - 汪辰 - 20200805 - PLCT实验室**

https://www.bilibili.com/video/BV1PA411Y7mz

**闪电：AOSP移植RISC-V有多难 - 吴伟 - V8技术讨论会 - OSDT社区 - 20200607**

https://www.bilibili.com/video/BV1wC4y1a7Za

**AOSP for RISC-V 移植教程之 Android Runtime 介绍 - 汪辰 - 20200814 - PLCT实验室**

https://www.bilibili.com/video/BV1wC4y1t7Xa

**AOSP for RISC-V 移植教程之 开始 ART 移植 - 汪辰 - 20200821 - PLCT实验室**

https://www.bilibili.com/video/BV1JK411M7e5

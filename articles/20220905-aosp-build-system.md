![](./diagrams/android.png)

文章标题：**Andorid Build System 研究心得**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

Android 的 Build System 一直是特别让人头痛的存在。我以前看的时候主要特别关注 Soong 的部分，但现在发现对整体的处理其实理解得还不够，这里的笔记就是希望从 Build Sytem 的整体角度有个更深刻的理解。

文章大纲

<!-- TOC -->

- [1. 参考](#1-参考)
- [2. 没有引入 Soong 之前的 Android Build System](#2-没有引入-soong-之前的-android-build-system)
- [3. 引入 Soong 后的变化](#3-引入-soong-后的变化)

<!-- /TOC -->

# 1. 参考

- 【参考 1】 Embedded Android, Karim Yagbmour, 2013

# 2. 没有引入 Soong 之前的 Android Build System

我觉得 【参考 1】写得挺好的，虽然这本书成书比较老了，还是基于 Android 2.3 和 4.1 写的，但是还是值得学习，特别是对于我这种半路出家的人，了解一下历史，才能更好地理解现在。

下图引用自 【参考 1】 Chapter 4 The Build System 的 “Figure 4-1. Android’s build system”

![](./diagrams/20220905-aosp-build-system/android-build-system-make.png)

【图 1】 早期基于 make 的 Android Build System

早期的 Android Build System 基于 make/Makefile 搭建，比较单纯，还是很好理解的。和目前这种半吊子的混合结构相比，难怪 Google 自己都看不下去，要用 Basel 代替，具体参考另一篇笔记 [《Bazel 和 AOSP 介绍》][1]

首先值得重点了解的一点是：**Android 构建系统不依赖于递归 makefiles，当我们执行 make 的时候会将所有的 `.mk` 文件都 include 到一个单个 Makefile 文件中**。因此，我们在 AOSP 源码树下看到的每个 `.mk` 文件最终都会成为单个巨大文件的一部分，而这个最终的大文件包含了用于构建的所有的各种变量定义和规则定义。但在实践中，这些文件是被拆分成很多文件，以模块化的方式分别开发的，粘合在一起的方式主要就是采用的 make 的 include 语法。只是这些文件的 include 的顺序有点讲究，后 include 的文件中的变量会依赖于先 include 的文件中的变量。

基于以上框架，对于一个 AOSP 系统（等同于 Ubuntu 发布包）中的众多软件包，在 AOSP 中称之为模块（module），由于不会像普通 GNU 软件的 Build System 那样采用递归方式进行构建，所以并不会为这些 module 提供自己的 Makefile 文件，而是替换为各自 module 的 `Android.mk` 文件。在我们执行 make 时利用会主动扫描 AOSP 源码树，获取所有的 `Android.mk` 文件后在逐个 include 进来。具体可以参考 【参考 1】 中 Chapter 4 中 “Why Does make Hang?” 部分的介绍。

作为外部 user，基于 Android 的 Build System 这个框架，我们要做的是客制化如下内容：

- 产品级别的内容，也就是 【图 1】 中的 Product Description 和 Board Description 部分的内容，通过对产品进行定义，主要是通过客制化一些变量告诉我们的 Build System 一款 product 自身的参数和构建的选项参数。这部分内容可以参考笔记 [《如何为 AOSP 的 lunch 新增一个菜单项》][2]
- 模块级别的构建规则，即 【图 1】 中的 Module Build Rules。大部分 AOSP 的 platform 中的 module 系统已经为我们写好了 Android.mk，但是如果我们需要客制化或者新增一些 module，也需要做相应的工作。

Build System 作为一个框架，它为使用者提供了如下的功能：

- 快速选择产品模板，有两种方法：一种是运行 lunch。运行 lunch 会将我们客制化的 product 信息列举出来，我们选择好后，可以快速加载 product 的参数变量。还有一种是将我们确定的 product 信息写入 `buildsepc.mk`。我们常用的方式是 lunch，这种方式执行后 product 的信息变量会以环境变量的形式被定义，然后在下一个阶段 make 过程中被引用。

- 执行构建，也就是当我们输入 make 时发生了什么。入口是 `core/main.mk`, 这个 makefile 文件会依次导入如下内容（注，这里仅列出重要的，不重要的内容太多，就不一一列举了）
  - 导入 `core/config.mk`，导入过程中会对用户选择的 product 的基本信息，如果是 lunch 方式则会读取环境变量中的 product 的参数值，对其进行检查和过滤，并生成最终的 make 内部的 build 变量。
    
    **值得一提的是，导入 config.mk 过程中还会导入 product 和 board 的配置信息，这个过程中会将用户客制化的产品级别信息导入进来。** ，见上面客制化工作的产品级别的内容。这涉及 `core/product_config.mk` 和 `core/board_config.mk`
    
  - 导入辅助函数，主要是对应 【图 1】 中的 `core/definitions.mk`
  - **导入系统的模块定义文件**。即导入 module 的 `Android.mk` 文件。见上面客制化工作的模块级别的构建规则的内容。这个 【图 1】中没有列出，这个过程比较费时。运行 make 时会看到提示 `Inclduing xxx.mk`。
  - 导入大量的构建规则，这个指的是 `core/Makefile`
  - 定义了大量的伪规则，这个基本上定义在 `core/main.mk` 的后半部分。

- 预定义了大量用于构建各种类型 module 的规则模板，体现在 【图 1】 中的 Module Build Templates，譬如：
  ```makefile
  CLEAR_VARS :=$= $(BUILD_SYSTEM)/clear_vars.mk
  BUILD_HOST_STATIC_LIBRARY :=$= $(BUILD_SYSTEM)/host_static_library.mk
  BUILD_HOST_SHARED_LIBRARY :=$= $(BUILD_SYSTEM)/host_shared_library.mk
  ```
  这些变量实际上对应 core 目录下的一些 .mk 文件，而用户在客制化自己的 Android.mk 文件时会 include 这些变量，其实相当于会把这些构建规则导入进来，这样 make 就知道怎么构建这些 module 了。
  

# 3. 引入 Soong 后的变化

仅仅是按照我自己的理解总结一下，仅供参考。以 AOSP 12 (tag: android-12.0.0_r3) 为参考代码基线。

我目前理解一个完整的构建和原来的差异不大，因为引入 Soong 后，只是把部分 module 的 `Android.mk` 替换为了 `Android.bp`。系统 image 的制作部分还是基于 Makefile 的，当然和最最初的区别是不是直接将 Makefile 传入 make 执行，而是交给 Kati 转化成 ninja 文件，building action 的最终动作是由 ninja 来驱动完成的。

所以还是结合 【图 1】 来看 AOSP 引入 Soong 后的变化：

- Product Descriotions 和 Board Description 部分没有什么变化
- Module Build Rules 这块，大部分 module 的 `Android.mk` 被转化成了 `Android.bp`
- Build System 部分，引入了 `soong_ui`, 这个 `soong_ui` 封装了一个新的构建过程，可以参考 `soong/ui/build/build.go` 这个文件中的 `Build` 函数，其核心执行顺序如下（不完整，抽取了核心步骤）：
  
  - runSoong：这是 Soong 新引入的步骤，会扫描所有的 `Android.bp` 转化为 ninjia 文件，为了和原有的 build system 融合，同时还会生成一个 `$(SOONG_ANDROID_MK)` 文件，譬如：`out/soong/Android-sdk_phone64_riscv64.mk`。这是一个很大的文件，打开看一下会发现这是一个符合 Android.mk 格式的形式的文件，但不是只针对一个 module，而是包含了所有解析过的 bp 文件中针对所有的 module 的符合 `Android.mk` 的定义，可以认为是那些采用 `Android.bp` 文件的 module 所对应的多个 `Android.mk` 文件的一个集合。摘录部分内容如下：
  
    ```
    LOCAL_MODULE_MAKEFILE := $(lastword $(MAKEFILE_LIST))
    
    include $(CLEAR_VARS)
    LOCAL_PATH := system/sepolicy
    LOCAL_MODULE := 26.0.compat.cil
    LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_unencumbered
    LOCAL_LICENSE_CONDITIONS := notice unencumbered
    LOCAL_NOTICE_FILE := system/sepolicy/NOTICE
    LOCAL_LICENSE_PACKAGE_NAME := system_sepolicy_license
    LOCAL_MODULE_CLASS := ETC
    LOCAL_PREBUILT_MODULE_FILE := out/soong/.intermediates/system/sepolicy/26.0.compat.cil/android_common/gen/26.0.compat.cil
    LOCAL_FULL_INIT_RC := 
    LOCAL_MODULE_PATH := out/target/product/emulator_riscv64/system/etc/selinux/mapping
    LOCAL_INSTALLED_MODULE_STEM := 26.0.compat.cil
    include $(BUILD_PREBUILT)
    
    include $(CLEAR_VARS)
    LOCAL_PATH := system/sepolicy
    LOCAL_MODULE := 26.0.ignore.cil
    LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_unencumbered
    LOCAL_LICENSE_CONDITIONS := notice unencumbered
    LOCAL_NOTICE_FILE := system/sepolicy/NOTICE
    LOCAL_LICENSE_PACKAGE_NAME := system_sepolicy_license
    LOCAL_MODULE_CLASS := ETC
    LOCAL_PREBUILT_MODULE_FILE := out/soong/.intermediates/system/sepolicy/26.0.ignore.cil/android_common/gen/26.0.ignore.cil
    LOCAL_FULL_INIT_RC := 
    LOCAL_MODULE_PATH := out/target/product/emulator_riscv64/system/etc/selinux/mapping
    include $(BUILD_PREBUILT)
    ```
  
  - runKatiBuild，这个函数封装了 Kati 处理，传入的是原来 make 的入口 `core/main.mk`，此时我们再看一下`core/main.mk`， 会发现这个 makefile 文件中针对 Soong 是有相关改动的，特别是是针对导入 module 的步骤，除了导入那些还没有采用 `Android.bp` 的 module 所对应的 `Android.mk` 外，还会导入一个特殊的 `$(SOONG_ANDROID_MK)` 文件，见上一步 runSoong 中的骚操作。所以说这里导入 module 时依然是一个全集，只是部分已经迁移到 soong 了，所以采用 `$(SOONG_ANDROID_MK)` 文件过渡了一下。
  - createCombinedBuildNinjaFile：合并 soong 和 kati 的 ninja 文件
  - runNinjaForBuild：执行 ninja

目前大致的理解如上，写得比较粗糙，可能还有一些细节，后面再慢慢补充。


[1]: ./20220615-introduce-bazel-for-aosp.md
[2]: ./20220315-howto-add-lunch-entry.md


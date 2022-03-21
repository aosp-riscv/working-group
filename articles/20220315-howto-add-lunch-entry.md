![](./diagrams/android-riscv.png)

文章标题：**如何为 AOSP 的 lunch 新增一个菜单项**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 如何增加一个普通的产品定义](#1-如何增加一个普通的产品定义)
- [2. 如何增加一个 GSI 的产品定义](#2-如何增加一个-gsi-的产品定义)
- [3. lunch 过程中 Android Build System 是如何识别产品的](#3-lunch-过程中-android-build-system-是如何识别产品的)
- [4. 总结：](#4-总结)

<!-- /TOC -->

注：本文内容基于 AOSP 12 (tag: `android-12.0.0_r3`)

# 1. 如何增加一个普通的产品定义

本章节是对以下参考文章的内容整理。
- [Adding a New Device](https://source.android.google.cn/setup/develop/new-device)
- [Understanding 64-bit Builds](https://source.android.google.cn/setup/develop/64-bit-builds)


所谓普通的 product，指的是在 AOSP 源码树中 `device` 目录下定义的一些产品

- 第(1)步：创建一个 `device/<company-name>/<device-name>` 目录, 譬如 `device/google/sunfish/`。

- 第(2)步：创建 DEVICE 级别的配置文件（`device.mk`），用来声明设备级别的信息。譬如 `device/google/sunfish/device-sunfish.mk`。sunfish 是一种 board/device 级别的概念，基本上代表了一款硬件，定义了板子上各种外设。我们可以简单看一下 `device-sunfish.mk` 文件的内容：

   ```makefile
   PRODUCT_HARDWARE := sunfish

   include device/google/sunfish/device-common.mk

   # 省略 ......
   ```

   这个文件又会 include 其他的 mk 文件，譬如 `device-common.mk`, 继续看看
   
   ```makefile
   # define hardware platform
   PRODUCT_PLATFORM := sm7150

   include device/google/sunfish/device.mk

   # 省略 ......
   ```
   感觉都是对硬件的一些配置和定义

- 第(3)步：创建 PRODUCT 级别的配置文件。PRODUCT 的概念可以理解成是在 DEVICE 更上一层的概念，譬如基于一款 board/device 定义的一款产品，譬如针对不同的国家。地区和运营商的定义。官方文档中的解释是:
  
  > Create a product definition makefile to create a specific product based on the device. 
  
  例如：`device/google/sunfish/aosp_sunfish.mk`。 

  我们查看这个文件发现其内容定义更侧重软件模块，主要是在描述该产品对应的文件系统 image 中会包含哪些模块和软件包等。

  在实际的产品开发中，产品的变化可能会十分丰富，所以有时候我们还会定义一些 base product，定义一些公共的产品特性，然后在这个基础上再定义派生的产品。

  > A common method is to create a base product that contains features that 
  > apply to all products, then create product variants based on that base product. 
  > For example, two products that differ only by their radios (CDMA versus GSM) 
  > can inherit from the same base product that doesn't define a radio.`

  产品的 mk 文件中可以定义的 `PRODUCT_*` 的变量。参考 <https://source.android.google.cn/setup/develop/new-device#prod-def>。 特别注意一个系统预定义的变量 `PRODUCT_DEVICE`
  
  > PRODUCT_DEVICE：Name of the industrial design. This is also the board name, 
  > and the build system uses it to locate BoardConfig.mk.`。

  我们来简单看看 `device/google/sunfish/aosp_sunfish.mk`，注意其中定义的 `PRODUCT_DEVICE := sunfish`
  
  ```makefile
  #
  # All components inherited here go to system image
  #
  $(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
  $(call inherit-product, $(SRC_TARGET_DIR)/product/generic_system.mk)

  # 省略 ......

  PRODUCT_MANUFACTURER := Google
  PRODUCT_BRAND := Android
  PRODUCT_NAME := aosp_sunfish
  PRODUCT_DEVICE := sunfish
  PRODUCT_MODEL := AOSP on sunfish
  ```

- 第(4) 步：创建一个板级配置文件 `BoardConfig.mk` 例如：`device/google/sunfish/sunfish/BoardConfig.mk`，这个文件中定义了更多 board 级别的编译构建变量定义。

  ```makefile
  # 省略 ......
  include device/google/sunfish/BoardConfig-common.mk
  ```

  感兴趣的可以去看看 `BoardConfig-common.mk` 这个文件。

- 第 (5) 步：上面这些准备工作做好后，我们需要在运行 lunch 命令时在下拉菜单列表中出现和我们的产品项对应的 entry item。具体防范就是创建一个 AndroidProducts.mk 文件。譬如 `device/google/sunfish/AndroidProducts.mk`。这是个入口文件，定义了我们在 lunch 的菜单列表中显示的项目名称以及前面第（3）步定义的 product 文件的位置。

  我们可以看看这个 `device/google/sunfish/AndroidProducts.mk` 文件：
  ```makefile
  PRODUCT_MAKEFILES := \
    $(LOCAL_DIR)/aosp_sunfish.mk \
    $(LOCAL_DIR)/aosp_sunfish_hwasan.mk \

  COMMON_LUNCH_CHOICES := \
    aosp_sunfish-userdebug \
  ```

  `COMMON_LUNCH_CHOICES` 中的字符串项会出现在 lunch 命令的项目列表中。
  
基于以上文件，AOSP 就可以根据我们在 lunch 过程中选择的 product-variant 找到 product 的相关配置文件。步骤大致如下：
- step 1：假设我们选择了 `aosp_sunfish-userdebug` 这一项，则 build 系统会根据 `-` 分离两个部分，前半部分是 `TARGET_PRODUCT`，即这里的 `aosp_sunfish`, 后半部分是 `TARGET_BUILD_VARIANT`, 即这里的 `userdebug`。有关 build variant 的定义，可参见 <https://source.android.google.cn/docs/setup/develop/new-device#build-variants>。
- step 2：然后再到 `PRODUCT_MAKEFILES` 中去匹配，根据 `TARGET_PRODUCT` 找到对应的 product 的 mk 文件，也就是我们在第（3）步中创建的 `device/google/sunfish/aosp_sunfish.mk`。
- step 3：找到 `device/google/sunfish/aosp_sunfish.mk` 后，在根据该文件中定义的 `PRODUCT_DEVICE` 的值就可以继续定位到 sunfish 目录，以及该目录下 product 对应的 board 级配置文件，譬如 `device-sunfish.mk` 和 `BoardConfig.mk`。

形象如下图所示，帮助理解整个过程。

![](./diagrams/20220315-howto-add-lunch-entry/normal-product.png)

# 2. 如何增加一个 GSI 的产品定义

device 目录下的那些产品对应的是具体的一个厂家的设备，Google 还提供了一些官方的 Generic System Images，有关 GSI 可以参考 [Generic System Images](https://source.android.google.cn/setup/build/gsi)。

这些 product 和具体的硬件没有关系，不包含 vendor 相关的内容，只含有 aosp 官方源码下的软件。

这些 GSI product 的定义和我们定义一个普通设备 product 的过程是一致的，以 arm64 为例，和我们前面举例的产品 sunfish 对应关系简单总结如下：

|                 | 普通产品定义                                   | GSI 产品定义                                          |
|-----------------|-----------------------------------------------|------------------------------------------------------|
|Product entry    | `device/google/sunfish/AndroidProducts.mk`    |`build/make/target/product/AndroidProducts.mk`        |
|PRODUCT Config   | `device/google/sunfish/aosp_sunfish.mk`       |`build/make/target/product/aosp_arm64.mk`             |
|Board/BSP config | `device/google/sunfish/sunfish/BoardConfig.mk`|`build/make/target/board/generic_arm64/BoardConfig.mk`|
|DEVICE Config    | `device/google/sunfish/device-sunfish.mk`     |`build/make/target/board/generic_arm64/device.mk`     |

注意不像普通产品，针对每款产品，譬如 sunfish，我们会在 device 的厂家（google）下新建一个独立的产品目录（sunfish）。所有的 GSI 产品的配置文件统一放在 `build/make/target` 下的 product 和 board 子目录下。

参考和对比以上关系，我们可以尝试为 riscv64 增加一款 GSI 产品

- 首先修改入口 `build/make/target/product/AndroidProducts.mk`，在 `COMMON_LUNCH_CHOICES` 中增加一项 `aosp_riscv64-eng`，这意味着 `TARGET_PRODUCT` 是 `aosp_riscv64`，`TARGET_PRODUCT_VARIANT` 是 `eng`。

- 然后增加一个对应的 product 的 mk 文件 `aosp_riscv64.mk`，放在 `build/make/target/product/` 下。同时将该文件的全路径添加到 `build/make/target/product/AndroidProducts.mk` 文件中的 `PRODUCT_MAKEFILES` 变量中。注意这个文件的名字要和 `TARGET_PRODUCT` 的一样。这样 aosp 的构建系统就会找到它了。

  aosp_riscv64.mk 的内容可以参考 aosp_arm64.mk，其中关键要知名如下内容：
  ```
  PRODUCT_NAME := aosp_riscv64
  PRODUCT_DEVICE := generic_riscv64
  PRODUCT_BRAND := Android
  PRODUCT_MODEL := AOSP on RISCV64
  ```

- 注意 `PRODUCT_DEVICE` 的值用于对应找到该产品对应的 BOARD 配置内容，为此我们还要在 `build/make/target/board` 下新建一个同名的 `generic_riscv64` 的目录，基本上一个 ARCH 就占一个目录，从 `generic_arm64` 下复制一份并改名。这个目录下一般需要这么几个文件：

  - README
  - device.mk : board 级别的 mk 文件
  - system.prop
  - BoardConfig.mk：这个 BoardConfig.mk 比较重要，这个文件中定义了更多 board 级别的编译构建变量定义。譬如：
    ```
    TARGET_ARCH := riscv64
    TARGET_ARCH_VARIANT := riscv64
    TARGET_CPU_VARIANT := generic
    TARGET_CPU_ABI := riscv64
    ```

主要就是以上修改，此时运行 lunch 应该就会看到 `aosp_riscv64-eng` 这一项，选择这一项后如果成功会打印出以下 product 配置信息。
```bash
============================================
PLATFORM_VERSION_CODENAME=REL
PLATFORM_VERSION=10
TARGET_PRODUCT=aosp_riscv64
TARGET_BUILD_VARIANT=eng
TARGET_BUILD_TYPE=release
TARGET_ARCH=riscv64
TARGET_ARCH_VARIANT=armv8-a
TARGET_CPU_VARIANT=generic
TARGET_2ND_ARCH=arm
TARGET_2ND_ARCH_VARIANT=armv8-a
TARGET_2ND_CPU_VARIANT=generic
HOST_ARCH=x86_64
HOST_2ND_ARCH=x86
HOST_OS=linux
HOST_OS_EXTRA=Linux-4.15.0-108-generic-x86_64-Ubuntu-18.04.4-LTS
HOST_CROSS_OS=windows
HOST_CROSS_ARCH=x86
HOST_CROSS_2ND_ARCH=x86_64
HOST_BUILD_TYPE=release
BUILD_ID=QQ3A.200605.002.A1
OUT_DIR=out
============================================
```

和普通的产品类似，基于以上文件，AOSP 就可以根据我们在 lunch 过程中选择的 product-variant 找到 product 的相关配置文件。步骤大致如下图所示，大家可以自行描述：

![](./diagrams/20220315-howto-add-lunch-entry/gsi-product.png)


# 3. lunch 过程中 Android Build System 是如何识别产品的

我们导入 build/envsetup.sh 后，就可以调用 lunch 命令。该命令的执行逻辑如下：

- print_lunch_menu，这个函数内部会触发 `out/soong_ui --dumpvar-mode COMMON_LUNCH_CHOICES`，打印输出我们看到的 product entry list
- 我们选择 product 后，lunch 继续 build_build_var_cache，此时获取我们选择的 product 的配置信息

更详细的处理可以参考另外两篇笔记
- [《envsetup.sh 中的 lunch 函数分析》][1]
- [《代码走读：对 soong_ui 的深入理解》][2]

# 4. 总结：

AOSP 中的产品定义文件是一些名为 `AndroidProducts.mk` 的文件，这些文件中关键是定义两个变量:

- `PRODUCT_MAKEFILES`: 定义真正的产品文件的路径，每个文件的命名必须符合 `$TARGET_PRODUCT.mk`
- `COMMON_LUNCH_CHOICES`: 定义可以出现在 lunch 菜单项中的条目，每个条目的命名格式是 `$TARGET_PRODUCT-$TARGET_BUILD_VARIANTS`


这些文件分两大类，存放在两个地方：

- 一类是 GSI 产品，由 Google 统一维护，统一存放在 `build/target/product/AndroidProducts.mk` 中。
- 一类是普通产品，由各个 vendor 维护，存放在 `device` 目录下，一般会按照 vendor 分目录分别存放。


[1]: ./20211026-lunch.mode
[2]: ./20211102-codeanalysis-soong_ui.md
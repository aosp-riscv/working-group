![](./diagrams/android.png)

文章标题：**为 AOSP 添加一个应用**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

整理了一些常用的模块配置脚本，方便以后参考和使用。

文章大纲

<!-- TOC -->

- [1. 参考](#1-参考)
- [2. 添加一个 Prebuilt APK](#2-添加一个-prebuilt-apk)
- [3. 添加一个 Native EXEC](#3-添加一个-native-exec)

<!-- /TOC -->

# 1. 参考

- 【参考 1】 Embedded Android, Karim Yagbmour, 2013
- 【参考 2】 [Soong Build System][1]
- 【参考 3】 [Build Cookbook][2], `offers code snippets to help you quickly implement some common build tasks`. 以及一些对 Android.mk Variables 的解释。

以下例子基于 AOSP for RISCV64，环境搭建参考 [Setup Android 12 on RISC-V][3]。

# 2. 添加一个 Prebuilt APK

这里演示如何在系统的 product 分区中添加一个提前编译好的 APK，所谓的提前编译好可以理解成不是在 AOSP 构建过程中编译生成，而是在 AOSP 源码树外编译生成 APK 文件，这里只是将该 APK 拷贝到 image 中。

假设 APK 的名字为 PrebuiltAPK

在 `packages/apps` 下创建一个 examples 目录，然后将 PrebuiltAPK.apk 拷贝到这个目录下，同时在该目录下创建一个 Android.mk 文件

```bash
$ mkdir -p packages/apps/examples
$ touch packages/apps/examples/Android.mk
$ cp some/place/PrebuiltAPK.apk packages/apps/examples/PrebuiltAPK.apk
```

编辑 `device/generic/goldfish/64bitonly/product/sdk_phone64_riscv64.mk`， 添加如下

```makefile
PRODUCT_PACKAGES += \
		PrebuiltAPK \
```

编辑 Android.mk， 参考 [例子](./code/20220908-add-app-in-aosp/PrebuiltAPK/Android.mk)

其中

`LOCAL_PRODUCT_MODULE := true` 确保生成的 PrebuiltAPK.apk 的位置在 product 分区下，输出路径为 `out/target/product/emulator_riscv64/product/app/PrebuiltAPK/PrebuiltAPK.apk`。如果不加这个，会报错如下：

```console
FAILED: 
build/make/core/artifact_path_requirements.mk:26: warning:  device/generic/goldfish/64bitonly/product/sdk_phone64_riscv64.mk produces files inside build/make/target/product/generic_system.mks artifact path requirement. 
Offending entries:
system/app/PrebuiltAPK/PrebuiltAPK.apk
In file included from build/make/core/main.mk:1342:
build/make/core/artifact_path_requirements.mk:26: error: Build failed.
17:15:16 ckati failed with: exit status 1
```

这个问题的原因参考 <https://source.android.com/docs/core/bootloader/partitions/product-interfaces#about-the-artifact-path-requirements>, 解决方法：<https://source.android.com/docs/core/bootloader/partitions/product-partitions#installing-to-product-partition>。

如果要添加的是 priv-app，则可以添加一个变量 `LOCAL_PRIVILEGED_MODULE := true`，输出路径为 `out/target/product/emulator_riscv64/product/priv-app/PrebuiltAPK/PrebuiltAPK.apk`


以上做完后，执行如下命令：
```bash
$ mmm packages/apps/examples/
$ m 
```


# 3. 添加一个 Native EXEC

- [Android.mk 的例子](./code/20220908-add-app-in-aosp/NativeEXEC/Android.mk)

- [Android.bp 的例子](./code/20220908-add-app-in-aosp/NativeEXEC/Android.bp)


[1]: https://source.android.com/docs/setup/build
[2]: https://wladimir-tm4pda.github.io/porting/build_cookbook.html
[3]: https://github.com/riscv-android-src/riscv-android/blob/main/doc/android12.md



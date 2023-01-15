![](./diagrams/android.png)

文章标题：**笔记：基于 Cuttlefish 调试 Android 内核**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 安装 Cuttlefish 运行环境](#2-安装-cuttlefish-运行环境)
- [3. 在 AOSP 中运行 Cuttlefish](#3-在-aosp-中运行-cuttlefish)
- [4. 利用 Cuttlefish 运行和调试 Android 的内核](#4-利用-cuttlefish-运行和调试-android-的内核)

<!-- /TOC -->

# 1. 参考文档 

- [1] [Cuttlefish and Kernel Dev, LPC 2022][1]
- [2] [Building Kernels][5] 官网构建 Android Kernel 的指导说明

# 2. 安装 Cuttlefish 运行环境

有关如何搭建 Cuttlefish 的运行环境，请参考另外一篇笔记 [《搭建 Cuttlefish 运行环境》][2]

# 3. 在 AOSP 中运行 Cuttlefish

AOSP 的构建参考另外一篇笔记 [《利用国内 mirror 搭建 AOSP 的 build 环境》][3]。

和 Goldfish（即我们在 Android SDK 中使用的 Android emulator）不同，Cuttlefish 的代码目前随 AOSP 一起发布，所以能够非常方便地保证和 AOSP 主线的一致性并采用和主线 AOSP 版本配套的 Cuttlefish 来测试 AOSP 主线的代码。在[《搭建 Cuttlefish 运行环境》][2] 中我们采用的方式是直接从 [Android CI 网站][4] 下载每日构建的 "aosp_cf_xxx" 的 product 的 system image 以及对应的 cvd-host_package 来测试。同样的，我们也可以自己基于 AOSP 的某个版本构建 system image 和与其对应的 cvd-host_package 来进行测试。方法步骤如下（以 `aosp_cf_x86_64_phone` 这个 product 的 userdebug 版本为例 ）：

假设 AOSP 的源码路径在 `<AOSP>`。

```bash
$ mkdir <AOSP> && cd <AOSP>
$ repo init -u https://android.googlesource.com/platform/manifest -b master
$ repo sync -j
$ source build/envsetup.sh
$ lunch aosp_cf_x86_64_phone-userdebug
$ m -j
```

构建完成后，除了在 `<AOSP>/out/target/product/vsoc_x86_64/` 下生成了于我们从 [Android CI][4] 下载的 `aosp_cf_x86_64_phone-img-xxxxxx.zip` 对应的一众 *.img 之外，从 [Android CI][4] 下载的 `cvd-host_package.tar.gz` 中的那些 bin 文件则生成在 `<AOSP>/out/host/linux-x86/bin` 下。

如果我们想调用 cuttlefish 运行我们的 AOSP，在 <AOSP> 路径下直接执行 `launch_cvd --daemon` 即可启动当前编译的 AOSP。其他 adb 和 WebUI 的访问方式和[《搭建 Cuttlefish 运行环境》][2] 中的一样。

在这种运行环境下，如果要查看 Cuttlefish 的 log，可以如下访问：

```bash
$ vi ~/cuttlefish_runtime/kernel.log
$ vi ~/cuttlefish_runtime/launcher.log 
$ vi ~/cuttlefish_runtime/logcat
```

# 4. 利用 Cuttlefish 运行和调试 Android 的内核

先构建 Android 的内核。参考 [2]。

这里我们使用最新的 common-android-mainline 分支。

注意从 Android 13 及以上开始，构建 Android Kernel 转为采用 Bazel(Kleaf)。

假设 Android Kernel 的源码路径在 `<AOSP-KERNEL>`

```bash
$ mkdir <AOSP-KERNEL> && cd <AOSP-KERNEL>
$ repo init -u https://android.googlesource.com/kernel/manifest -b \
common-android-mainline
$ repo sync -j
$ tools/bazel run //common:kernel_x86_64_dist
$ tools/bazel run //common-modules/virtual-device:virtual_device_x86_64_dist
```

构建完成后，我们会在 `<AOSP-KERNEL>/out/android-mainline/dist/` 下看到生成的内核文件 `bzImage` 以及对应的 ramdisk 文件 `initramfs.img`。

参考上一章节，返回 AOSP 源码路径，尝试用我们的新内核启动 AOSP

```bash
$ cd <AOSP>
$ source ./build/envsetup.sh
$ lunch aosp_cf_x86_64_phone-userdebug
$ launch_cvd -kernel_path <AOSP-KERNEL>/out/android-mainline/dist/bzImage \
-initramfs_path <AOSP-KERNEL>/out/android-mainline/dist/initramfs.img  --daemon
```

启动完成后，可以 adb 登录看一下内核版本：

```bash
$ adb shell
vsoc_x86_64:/ $ uname -a
Linux localhost 6.1.0-mainline-maybe-dirty #1 SMP PREEMPT Thu Jan  1 00:00:00 UTC 1970 x86_64 Toybox
```

可以看到内核已经换成了我们自己编译的最新的内核版本了。

下面来继续尝试在 Cuttlefish 的基础上利用 gdb 调试一下我们的内核。

```bash
$ cd <AOSP>
$ source ./build/envsetup.sh
$ lunch aosp_cf_x86_64_phone-userdebug
$ launch_cvd -kernel_path <AOSP-KERNEL>/out/android-mainline/dist/bzImage \
-initramfs_path <AOSP-KERNEL>/out/android-mainline/dist/initramfs.img \
-gdb_port 1234 -cpus=1 \
-extra_kernel_cmdline nokaslr
```

然后新开一个终端就可以使用 gdb 进行内核调试了。

```bash
$ cd <AOSP-KERNEL>/common
$ gdb ../out/android-mainline/dist/vmlinux
GNU gdb (Ubuntu 9.2-0ubuntu1~20.04.1) 9.2
Copyright (C) 2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from ../out/android-mainline/dist/vmlinux...
(gdb) target remote :1234
Remote debugging using :1234
0x000000000000fff0 in exception_stacks ()
(gdb) hbreak start_kernel
Hardware assisted breakpoint 1 at 0xffffffff83039c49: file init/main.c, line 935.
(gdb) c
Continuing.

Breakpoint 1, start_kernel () at init/main.c:935
935	{
(gdb) 
```




[1]:https://lpc.events/event/16/contributions/1332/
[2]:./20230111-cuttlefish-setup.md
[3]:./20230111-aosp-build.md
[4]:https://ci.android.com/
[5]:https://source.android.com/docs/setup/build/building-kernels
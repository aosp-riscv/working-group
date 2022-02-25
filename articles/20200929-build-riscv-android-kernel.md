![](./diagrams/android-riscv.png)

文章标题：**编译一个 RISC-V 的 Android 内核**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 前言](#1-前言)
- [2. 下载 Android 内核源码](#2-下载-android-内核源码)
- [3. 下载 Android 内核配置](#3-下载-android-内核配置)
- [4. 编译 Android 内核](#4-编译-android-内核)

<!-- /TOC -->

# 1. 前言

今天尝试了一下将 Android 的内核交叉编译运行在 RISC-V 平台上，笔记小结如下，如果有什么补
充欢迎留言。
Google 官方提供了编译内核的指导，参考 <https://source.android.google.cn/setup/build/building-kernels>。
但这个 build 过程是针对 Google 的官方编译，也就是说其操作使用的是 LLVM/Clang 编译器，
而且只支持 ARM/X86/... 几个有限的平台。而我这里计划是要将其 port 到 RISC-V 上，所以直
接使用是不行的。研究了一下打算按下面的步骤来做。

有关针对 RISC-V 的 GNU toolchain（注：这里暂不使用 LLVM/Clang，使用 LLVM/Clang 放到
后续尝试），QEMU 和文件系统等的制作方法请参考过去我在知乎上总结的文章：
[汪辰：在 QEMU 上运行 RISC-V 64 位版本的 Linux](https://zhuanlan.zhihu.com/p/258394849)。
本文要做的事情实际上就是把其中制作内核部分的操作换掉，因为这次我们要编译一个 Andorid 的 Linux 内核。


# 2. 下载 Android 内核源码

Andorid 的官方内核源码下载请访问 <https://android.googlesource.com/kernel/common/>，
如果这个访问不了，可以访问 github: <https://github.com/aosp-mirror/kernel_common>，
或者尝试国内的一些 mirror，譬如
[Tsinghua Open Source Mirror](https://mirrors.tuna.tsinghua.edu.cn/help/AOSP)。
clone 下来后切换到 android-5.4-stable 分支（commit：bb168ca1805b33289aae802a940551dec93a24d3），
这个分支也是撰写本文时 AOSP 官方维护的最新的 LTS 稳定分支，值得信赖。

总结命令如下：
```
$ git clone https://android.googlesource.com/kernel/common
$ cd common
$ git checkout android-5.4-stable
```

根据我在另一篇文章: [AOSP 内核的版本管理](./20200915-android-linux-version.md) 中的
介绍，我们知道，`android-mainline` 是 Android 的主要开发分支，Google 在这个主分支上密
切跟踪 upstream Linux 的 mainline。当 Linux 宣布某个 mainline 版本成为一个新的 LTS 
版本时，Google 将相对应地以 `android-mainline` 分支为基础拉一个 AOSP common kernel 
分支并积极维护这个分支，在这里就是 `android-5.4-stable`，这个分支基于最新的 LTS 5.4.x 
并包含有所有特定于 Android 的补丁。查看内核源码根目录下的 Makefile 文件，我们可以知道这
个分支目前基于的是 5.4.x LTS 的小版本 5.4.61，应该是比较新的了。

```
$ cat Makefile | head
# SPDX-License-Identifier: GPL-2.0
VERSION = 5
PATCHLEVEL = 4
SUBLEVEL = 61
EXTRAVERSION =
NAME = Kleptomaniac Octopus

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be located in ./README
```

# 3. 下载 Android 内核配置

但光是源码中含有 Android 的补丁还不够，Google 对 Android 的内核还有自己特定的配置，
所以我们还需要把这些配置加上。

我们知道对于一款设备或者一个平台，内核中一般都提供了它的缺省配置，譬如我们在前面配置 
RISC-V 的内核时就是使用了它的缺省配置，当时我们配置操作命令如下，
参考：[《QEMU 上运行 RISC-V 64 位版本的 Linux》](https://zhuanlan.zhihu.com/p/258394849)。

```
$ make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- defconfig
```

而这个 defconfig 就是对应着内核源码树下 `arch/riscv/configs/` 目录下的 defconfig 文件

```
$ ls arch/riscv/configs/ -l
total 8
-rw-rw-r-- 1 u u 2144 9月  18 12:13 defconfig
-rw-rw-r-- 1 u u 2102 9月  18 12:13 rv32_defconfig
```

如果要支持特定的硬件或软件功能（譬如这里要专门针对 Android），则我们可以为 defconfig 
文件打补丁，这些补丁文件我们术语上称之为配置 “片段（fragments）”。这些 fragments 文件
的格式与 defconfig 相同，但通常要小得多，因为它们仅包含为支持特定的硬件或软件功能/行为
所需的内核配置项。我们可以使用内核提供的合并脚本（即内核源码树中 `scripts/kconfig/merge_config.sh` 
将这些 fragments 文件与平台的 defconfig 文件进行合并。

Android 内核的 fragments 配置专门维护在一个独立的仓库中，官网地址是：
<https://android.googlesource.com/kernel/configs>。github 和国内 mirror 上似乎还没
看到比较官方的 mirror，有知道的同学请留言，谢了先。

下载这个仓库：
```
$ git clone https://android.googlesource.com/kernel/configs
$ cd configs
```

可以仔细读一下仓库根目录下的 README.md 文件。
它告诉我们 android 内核的 fragments 配置分几类:

- `android-base.config`: 这是 Android 的基本配置，必须要应用。
- `android-recommended.config`: 这些配置属于对 Android 功能增强，推荐使用但不强制使用。
- `android-base-conditional.xml`：在 Android P（包括 Android P）以下的版本中，特定于
  体系结构的内核配置要求包含在特定于体系结构的基本配置 fragments 中，例如 
  `android-base-arm64.config`。但在 Android P 之后的版本中，特定于体系结构的基本配置 
  fragments 被删除，一些可选的内核配置要求存储在 `android-base-conditional.xml` 文件
  中，注意这里变成了 xml 的格式，也就是说不能直接使用 merge 脚本和 defconfig 合并，
  不知道 Google 为何这么做，也许是为了强迫大家不要图省事，不得不从 xml 文件中手动挑选合
  适的配置项进行添加。
- `non_debuggable.config`：其他用户自己的配置项。

这个 configs 仓库也包含了很多个分支，针对一个正式的 Andorid 版本有对应的发布分支，譬如
最新的 Android 11 其发布分支就是 `android11-release`（commit：2f1dcde47942beb27201574a0c09bbf4182b79e6）

```
$ git checkout android11-release
Switched to branch 'android11-release'
Your branch is up to date with 'origin/android11-release'.
$ ls -l
total 44
drwxrwxr-x 2 u u  4096 9月  18 10:45 build
drwxrwxr-x 5 u u  4096 9月  18 10:45 o
drwxrwxr-x 5 u u  4096 9月  18 10:45 o-mr1
-rw-rw-r-- 1 u u   112 9月  18 10:45 OWNERS
drwxrwxr-x 5 u u  4096 9月  18 10:45 p
drwxrwxr-x 5 u u  4096 9月  18 10:45 q
drwxrwxr-x 5 u u  4096 9月  18 10:45 r
-rw-rw-r-- 1 u u 10552 9月  18 10:45 README.md
drwxrwxr-x 2 u u  4096 9月  18 10:45 tools
```

切换到这个分支后我们看到一些 o、p、q、r 这样的目录，这些就是对应着 Android platform version：8、9、10、11。
进去看一下

```
$ cd p
$ ls -l
total 16
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-4.14
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-4.4
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-4.9
-rw-rw-r-- 1 u u  218 9月  18 09:44 README.md
$ cd ../q
$ ls -l
total 12
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-4.14
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-4.19
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-4.9
$ cd ../r
$ ls -l
total 12
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-4.14
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-4.19
drwxrwxr-x 2 u u 4096 9月  18 10:45 android-5.4
```

每个 platform 下对应三个内核版本，回忆一下 [《AOSP 内核的版本管理》](./20200915-android-linux-version.md) 
中章节 “Android Platform 的内核版本管理” 的介绍会进一步加深 Google 对 Android 内核版本的管理方式。
configs 仓库的 master 分支上维护着下一个 AOSP 的 Platform 版本（这里是 12/S）会支持的
内核版本。切换到 master 看一下：

```
$ git checkout master
$ ls -l
total 56
drwxrwxr-x 2 u u  4096 9月  24 16:02 android-4.14
drwxrwxr-x 2 u u  4096 9月  24 16:02 android-4.19
drwxrwxr-x 2 u u  4096 9月  24 16:02 android-5.4
......
```

看上去好像不对，但注意按照 google 的定义，4.14 肯定不会在 S 中被支持，而不确定的 5.x 
要到 2020 年底才会揭晓，所以现在只是还未确定，这个不矛盾。

# 4. 编译 Android 内核

理解了这些概念后我们可以开始干活了。关键是要把 Android 的配置 fragments 合并到缺省的针
对 RISC-V 平台的配置下去。假设 common 仓库目录和 configs 仓库目录和 riscv64-linux 目
录（有关 `riscv64-linux` 目录同样参考 
[《QEMU 上运行 RISC-V 64 位版本的 Linux》](https://zhuanlan.zhihu.com/p/258394849) 
一文）关系如下：

```
$ tree -L 2
.
├── aosp-kernel
│   ├── common
│   └── configs
└── riscv64-linux
    ├── busyboxsource
    ├── linux
    ├── qemu-5.1.0
    ├── riscv-gnu-toolchain
    ├── rootfs
    └── rootfs.img
```

首先进入 configs 仓库，切换为 `android11-release`

```
$ cd configs
$ git checkout android11-release
```

然后进入 common 仓库，先清理一下，然后确保切换到 `android-5.4-stable` 分支 再执行合并
配置操作，完成后直接编译即可，注意运行 `merge_config.sh` 脚本文件后会自动在内核源码根
目录下生成 `.config` 文件。注意我这里只合并了 base 配置。

```
$ cd common
$ make ARCH=riscv distclean
$ git checkout android-5.4-stable
$ ARCH=riscv scripts/kconfig/merge_config.sh arch/riscv/configs/defconfig ../configs/r/android-5.4/android-base.config
$ make ARCH=riscv CROSS_COMPILE=riscv64-unknown-linux-gnu- -j $(nproc)
```

QEMU 中启动后检查内核版本信息：

```
/ # uname -a
Linux (none) 5.4.61-00012-gbb168ca1805b #1 SMP PREEMPT Fri Sep 18 12:17:20 CST 2020 riscv64 GNU/Linux
```

可以看到内核已经变成 5.4.61，而且注意和 defconfig 编译出的内核的一个区别，就是上述信息
中出现了一个 `PREEMPT`，这说明内核的内核态抢占被打开了，这体现了 Android 对内核的特殊
要求，作为主要应用于移动通讯设备的 Linux 版本，为了保证一些实时应用，这个选项是必须的。
具体也可以查看 Android 的 android-base.config 配置 fragment 文件，中间的确有一行：

```
......
CONFIG_PREEMPT=y
......
```


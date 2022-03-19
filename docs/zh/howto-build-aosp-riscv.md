**如何编译 aosp-riscv**

注：提供此文档为方便国内参与小伙伴。

<!-- TOC -->

- [1. 硬件环境](#1-硬件环境)
- [2. 安装依赖软件](#2-安装依赖软件)
- [3. 安装 repo](#3-安装-repo)
- [4. 下载源码](#4-下载源码)
- [5. 编译](#5-编译)
- [6. 构建 Clang](#6-构建-clang)

<!-- /TOC -->

# 1. 硬件环境

本文所有操作在以下系统环境下验证通过

```
$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 18.04.6 LTS
Release:        18.04
Codename:       bionic
```

# 2. 安装依赖软件

```
$ sudo apt install git-core gnupg flex bison build-essential zip curl zlib1g-dev \
                   gcc-multilib g++-multilib libc6-dev-i386 libncurses5 \
                   lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z1-dev \
                   libgl1-mesa-dev libxml2-utils xsltproc unzip fontconfig python
```

# 3. 安装 repo

下载（参考 [清华大学开源软件镜像站 Git Repo 镜像使用帮助](https://mirrors.tuna.tsinghua.edu.cn/help/git-repo/)）

```
$ curl https://mirrors.tuna.tsinghua.edu.cn/git/git-repo -o repo
$ chmod +x repo
```
为了方便可以将其拷贝到你的 PATH 里。

repo 运行过程中会尝试访问官方的 git 源更新自己，如果想使用 tuna 的镜像源进行更新，可以
将如下内容复制到你的 `~/.bashrc` 里并重启终端生效。

安装好后可以运行 `repo version` 检查效果，出现类似以下输出说明安装成功。repo 版本需要 2.15 以上。

```
$ repo version
<repo not installed>
repo launcher version 2.17
       (from /home/wangchen/bin/repo)
git 2.17.1
Python 3.6.9 (default, Jan 26 2021, 15:33:00)
[GCC 8.4.0]
OS Linux 4.15.0-144-generic (#148-Ubuntu SMP Sat May 8 02:33:43 UTC 2021)
CPU x86_64 (x86_64)
Bug reports: https://bugs.chromium.org/p/gerrit/issues/entry?template=Repo+tool+issue
```

# 4. 下载源码

创建一个 AOSP 的源码构建目录，这里假设为 `/home/u/aosp`，然后进入 aosp 目录
```
$ mkdir -p /home/u/aosp
$ cd /home/u/aosp
```

进入 AOSP 构建目录后执行以下命令下载 AOSP 的源码

注：由于采用的是 Gitee 上的 aosp-riscv 开发仓库，以及采用国内清华源的 AOSP 镜像作为缺省 remote，将大大方便国内用户下载代码。

```
$ repo init -u git@gitee.com:aosp-riscv/platform_manifest.git -b riscv64-android-12.0.0_dev_cn
$ repo sync -j8
```

# 5. 编译

注意：因为还在开发过程中，当前还不支持完整的编译 `m`。

下面是一个例子，编译 aosp 但不运行最终的 ninja 构建。

```
$ cd /home/u/aosp
$ source ./build/envsetup.sh
$ lunch aosp_riscv64-eng
$ m --skip-ninja
```

# 6. 构建 Clang

```
$ mkdir llvm-toolchain && cd llvm-toolchain
$ repo init -u git@github.com:aosp-riscv/platform_manifest.git -b riscv64-llvm-master
$ repo sync -c
$ python toolchain/llvm_android/build.py
```
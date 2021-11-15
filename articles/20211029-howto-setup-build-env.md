**如何编译 aosp-riscv**
<!-- TOC -->

- [1. 硬件环境](#1-硬件环境)
- [2. 安装依赖软件](#2-安装依赖软件)
- [3. 安装 repo](#3-安装-repo)
- [4. 下载源码](#4-下载源码)
- [5. 下载 patch 并打补丁](#5-下载-patch-并打补丁)
- [6. 编译](#6-编译)

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

```
$ mkdir ~/bin
$ export PATH=~/bin:$PATH
$ export REPO=$(mktemp /tmp/repo.XXXXXXXXX)
$ curl -o ${REPO} https://storage.googleapis.com/git-repo-downloads/repo
$ gpg --keyserver keyserver.ubuntu.com --recv-key 8BB9AD793E8E6153AF0F9A4416530D5E920F5C65
$ curl -s https://storage.googleapis.com/git-repo-downloads/repo.asc | gpg --verify - ${REPO} && install -m 755 ${REPO} ~/bin/repo
```

其中 export 的 PATH 以后可以写入 `~/.bashrc` 文件中去，避免以后每次定义。

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

如果你的机器是在中国大陆内还需要定义并导出一个环境变量：`export REPO_URL='https://mirrors.tuna.tsinghua.edu.cn/git/git-repo'`，否则在后面执行 repo init 的时候会默认去 google 的网站下载 repo 的第二阶段，但由于网络原因会导致连接不上，所以改成国内的 git-repo 镜像地址。其中 export 的 PATH 以后可以写入 `~/.bashrc` 文件中去，避免以后每次定义。

# 4. 下载源码

创建一个 AOSP 的源码构建目录，这里假设为 `/home/u/aosp`，然后进入 aosp 目录
```
$ mkdir -p /home/u/aosp
$ cd /home/u/aosp
```

进入 AOSP 构建目录后执行以下命令下载 AOSP 的源码
```
$ repo init -u git@gitee.com:aosp-riscv/platform_manifest.git -b riscv64-android-12.0.0_dev
$ repo sync -j8
```

# 5. 下载 patch 并打补丁

由于部分 AOSP 仓库整体体积过大或者部分文件体积过大，超过 gitee/github 限制的，目前无法在 gitee/github 上新建仓库，所以目前采用补丁方式保存修改，这些补丁单独存放在一个仓库 <https://gitee.com/aosp-riscv/patches_aosp_riscv> 中。

**注意请将这个仓库下载在 AOSP 的构建目录（譬如这里是 `/home/u/aosp`）之外。**, 这里假设下载到 `/home/u` 下。

执行如下命令下载额外的补丁并执行脚本文件应用补丁。

```
$ cd /home/u
$ git clone git@gitee.com:aosp-riscv/patches_aosp_riscv.git
$ cd patches_aosp_riscv/
$ git checkout riscv64-android-12.0.0_dev
$ ./apply_patches.sh /home/u/aosp
```

# 6. 编译

```
$ cd /home/u/aosp
$ source ./build/envsetup.sh
$ lunch aosp_riscv64-eng
$ m --skip-ninja --skip-soong-tests
```

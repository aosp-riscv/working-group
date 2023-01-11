![](./diagrams/android.png)

文章标题：**笔记：利用国内 mirror 搭建 AOSP 的 build 环境**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 官方的 AOSP 构建环境参考](#1-官方的-aosp-构建环境参考)
- [2. 硬件要求](#2-硬件要求)
- [3. 操作系统](#3-操作系统)
- [4. 安装软件依赖](#4-安装软件依赖)
- [5. 安装 repo](#5-安装-repo)
- [6. 下载源码](#6-下载源码)
- [7. 构建](#7-构建)

<!-- /TOC -->

# 1. 官方的 AOSP 构建环境参考
- [1] [Requirements][5] 硬件和系统基本要求
- [2] [Establishing a build environment][1] 软件依赖安装
- [3] [Source control tools][2] repo/git 仓库管理软件介绍
- [4] [Downloading the Source][3] 介绍如何下载代码仓库
- [5] [Building Android][4] 介绍如何构建
- [6] [Android 镜像使用帮助][6]

注意，Google 官方的 android 网站 URL 前缀凡是 <https://source.android.com/> 的国内大陆无法直接访问，可以替换为 <https://source.android.google.cn/>。

同样官方下载网站 <https://android.googlesource.com/> 我们也是无法直接访问的，还好我们有国内的镜像，这篇笔记文章综合以上参考资料简单总结一下如果利用国内镜像搭建一个 AOSP 的构建环境，因为经常要搭建环境，记下来方便快速查看。

# 2. 硬件要求

- 64 位环境
- 如果要检出代码，至少需要 250 GB 可用磁盘空间；如果要进行构建，则还需要 150 GB。如果要进行多次构建，则需要更多空间。
- 至少需要 16 GB 的可用 RAM，但 Google 建议提供 64 GB。我这里测试发现 16 GB 明显是不够用的，目前我自己的机器上至少 32 GB。

以下信息供参考：从 2021 年 6 月起，Google 使用 72 核机器，内置 RAM 为 64 GB，完整构建过程大约需要 40 分钟（增量构建只需几分钟时间，具体取决于修改了哪些文件）。相比之下，RAM 数量相近的 6 核机器执行完整构建过程需要 3 个小时。

# 3. 操作系统

Google 官方推荐 Ubuntu。

我自己试过 Ubuntu 的以下 LTS 版本：18.04、20.04 以及 22.04，目前主要用 18.04 和 20.04，22.04 我用的较少。

# 4. 安装软件依赖

参考 [2]

```bash
sudo apt-get install git-core gnupg flex bison build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 libncurses5 lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z1-dev libgl1-mesa-dev libxml2-utils xsltproc unzip fontconfig
```

此外还需要安装 python， 否则 repo 无法运行

```bash
sudo apt install python
```

# 5. 安装 repo

参考 [6]，这里我们采用清华大学开源软件镜像站提供的软件包。

下载 repo 工具:

```bash
mkdir ~/bin
cd ~/bin
curl https://mirrors.tuna.tsinghua.edu.cn/git/git-repo -o repo
chmod a+x repo
```

为了方便可以将 repo 路径写入 `~/.bashrc` 的 PATH 里

```bash
export PATH=~/bin:$PATH
```

repo 运行过程中会尝试访问官方的 git 源更新自己，这导致在执行 `repo init` 的时候会去访问外网 google 的网站，因为无法访问而阻塞其行为。为避免该行为，可以将如下内容复制到你的 `~/.bashrc` 里使用 tuna 的镜像源进行更新，
```bash
export REPO_URL='https://mirrors.tuna.tsinghua.edu.cn/git/git-repo'
```

以上更新完 `~/.bashrc` 后记得 source 该文件或者重启终端。

安装好后可以运行 `repo version` 检查效果：

```bash
$ repo version
<repo not installed>
repo launcher version 2.30
       (from /home/u/bin/repo)
git 2.25.1
Python 3.8.10 (default, Nov 14 2022, 12:59:47) 
[GCC 9.4.0]
OS Linux 5.15.0-57-generic (#63~20.04.1-Ubuntu SMP Wed Nov 30 13:40:16 UTC 2022)
CPU x86_64 (x86_64)
Bug reports: https://bugs.chromium.org/p/gerrit/issues/entry?template=Repo+tool+issue
```

repo 版本需要 2.15 以上， 显示 `(from /home/u/bin/repo)` 说明是手动安装。

# 6. 下载源码

参考 [6]，这里我们采用清华大学开源软件镜像站提供的软件包。

由于采用 `repo init` 加 `repo sync` 的方法首次同步需要下载约 190GB 数据，过程中任何网络故障都可能造成同步失败，所以我按照 [6] 上建议的采用初始化包进行初始化。

使用方法如下:

```bash
# 下载初始化包
curl -OC - https://mirrors.tuna.tsinghua.edu.cn/aosp-monthly/aosp-latest.tar 
# 解压，解压得到 AOSP 工程目录
tar xf aosp-latest.tar
# 进入 AOSP 工程目录，这时 ls 的话什么也看不到，因为只有一个隐藏的 .repo 目录
cd AOSP
# 正常同步一遍即可得到完整目录
repo sync 
```
此后，每次只需运行 `repo sync` 即可保持同步。 

# 7. 构建

参考 [5]。


[1]:https://source.android.com/docs/setup/start/initializing
[2]:https://source.android.com/docs/setup/download
[3]:https://source.android.com/docs/setup/download/downloading
[4]:https://source.android.com/docs/setup/build/building
[5]:https://source.android.com/docs/setup/start/requirements
[6]:https://mirrors.tuna.tsinghua.edu.cn/help/AOSP/
**How to build aosp-riscv**

<!-- TOC -->

- [1. Hardware environment](#1-hardware-environment)
- [2. Install dependent software](#2-install-dependent-software)
- [3. Install repo](#3-install-repo)
- [4. Download source code](#4-download-source-code)
- [5. Build](#5-build)
- [6. Build Clang](#6-build-clang)

<!-- /TOC -->

# 1. Hardware environment

All operations in this article have been verified under the following system environment:

```
$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 18.04.6 LTS
Release:        18.04
Codename:       bionic
```

# 2. Install dependent software

```
$ sudo apt install git-core gnupg flex bison build-essential zip curl zlib1g-dev \
                   gcc-multilib g++-multilib libc6-dev-i386 libncurses5 \
                   lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z1-dev \
                   libgl1-mesa-dev libxml2-utils xsltproc unzip fontconfig python
```

# 3. Install repo

```
$ mkdir ~/bin
$ export PATH=~/bin:$PATH
$ export REPO=$(mktemp /tmp/repo.XXXXXXXXX)
$ curl -o ${REPO} https://storage.googleapis.com/git-repo-downloads/repo
$ gpg --keyserver keyserver.ubuntu.com --recv-key 8BB9AD793E8E6153AF0F9A4416530D5E920F5C65
$ curl -s https://storage.googleapis.com/git-repo-downloads/repo.asc | gpg --verify - ${REPO} && install -m 755 ${REPO} ~/bin/repo
```

`PATH` can be written into `~/.bashrc` to avoid definition everytime.

After the installation is complete, you can run `repo version` to check the effect. An output similar to the following shows that the installation is successful. The repo version needs to be 2.15 or higher.

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

# 4. Download source code

Create an AOSP source code build directory, here assume it is `/home/u/aosp`, and then enter the aosp directory.

```
$ mkdir -p /home/u/aosp
$ cd /home/u/aosp
```

After entering the AOSP build directory, execute the following command to download the source code of AOSP

```
$ repo init -u git@github.com:aosp-riscv/platform_manifest.git -b riscv64-android-12.0.0_dev
$ repo sync -j8
```

# 5. Build

Note: Because it is still in the development process, full compilation of `m` is not currently supported.

Following is just sample on how to build without ninja.

```
$ cd /home/u/aosp
$ source ./build/envsetup.sh
$ lunch aosp_riscv64-eng
$ m --skip-ninja
```

# 6. Build Clang

```
$ mkdir llvm-toolchain && cd llvm-toolchain
$ repo init -u git@github.com:aosp-riscv/platform_manifest.git -b riscv64-llvm-master
$ repo sync -c
$ python toolchain/llvm_android/build.py
```

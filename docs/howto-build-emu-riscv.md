**How to build android emulator for riscv**

<!-- TOC -->

- [1. Hardware environment](#1-hardware-environment)
- [2. Install dependent software](#2-install-dependent-software)
- [3. Install repo](#3-install-repo)
- [4. Download source code](#4-download-source-code)
- [5. Build](#5-build)
    - [5.1. Incremental builds](#51-incremental-builds)
    - [5.2. Speeding up builds with ‘ccache’](#52-speeding-up-builds-with-ccache)
- [6. Test with the generated AOSP system image](#6-test-with-the-generated-aosp-system-image)

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
$ sudo apt-get install -y git build-essential python qemu-kvm ninja-build \
                          python-pip ccache
```

# 3. Install repo

First we need to obtain the repo tool.

```
$ mkdir $HOME/bin
$ curl http://commondatastorage.googleapis.com/git-repo-downloads/repo > $HOME/bin/repo
$ chmod 700 $HOME/bin/repo
```

Make sure to add `$HOME/bin` to your path, if it is not already there.

```
export PATH=$PATH:$HOME/bin
```

Do not forget to add this to your `.bashrc` file.

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

Create an emulator source code build directory, here assume it is `/home/u/emu-master-dev`, and then enter the directory.

```
$ mkdir -p /home/u/emu-master-dev && cd /home/u/emu-master-dev
```

After entering the build directory, execute the following command to download 
the source code.

```
$ repo init -u git@github.com:aosp-riscv/platform_manifest -b riscv64-emu-master-dev
$ repo sync -j8
```

# 5. Build

```
$ cd external/qemu && android/rebuild.sh
```

If all goes well you should have a freshly build emulator in the objs directory.

You can pass the flag `--help` to the rebuild script to get an idea of which 
options you can pass in.

## 5.1. Incremental builds

The rebuild script does a complete clean build. You can use ninja to partial builds:

```
$ ninja -C objs
```

## 5.2. Speeding up builds with ‘ccache’

It is highly recommended to install the ‘ccache’ build tool on your development 
machine(s). The Android emulator build scripts will probe for it and use it if 
available, which can speed up incremental builds considerably.

```
sudo apt-get install ccache
```

# 6. Test with the generated AOSP system image

It is also possible to try booting an Android system image built from a fresh 
AOSP platform checkout. One has to select an emulator-compatible build product, 
e.g.:

```
$ cd $AOSP/
$ . build/envsetup.sh 
$ lunch sdk_phone_arm64-eng
$ make -j8
```

`$AOSP` is the path to your aosp source tree.

Recommended build products are 'sdk_phone_arm64-eng ' or 'sdk_phone_x86_64-eng'.

To boot the generated system image:

```
$ cd /home/u/emu-master-dev/external/qemu 
$ ./android/rebuild.sh 
$ export ANDROID_BUILD_TOP=/path/to/aosp
$ objs/emulator
```

Note:
- The above command lines to start the emulator must be in the same terminal
  session as the lunch command, otherwise an error will be reported.
- If you want to launch emulator without GUI(headless mode), you can add 
  `-no-window` option.
- If you see error: "pulseaudio: Failed to initialize PA contextaudio: Could not
  init `pa' audio driver", you can add `-no-audio` option.
- If you see error: "PCI bus not available for hda", you can add `-qemu -machine virt`.
- If you want to see kernel log in headless mode, you can add `-show-kernel` option.

To sum up, assuming that we have compiled and generated the aosp image of
`sdk_phone_arm64-eng`and emulator. When we want to run and test this aosp image
with our own compiled emulator, and use text mode (no GUI), you can enter the
following commands:
```
$ cd $AOSP/
$ . build/envsetup.sh
$ lunch sdk_phone_arm64-eng
$ cd /home/u/emu-master-dev/external/qemu
$ export ANDROID_BUILD_TOP=$AOSP
$ objs/emulator -no-window -show-kernel -no-audio -qemu -machine virt
```
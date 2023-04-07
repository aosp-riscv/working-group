![](./diagrams/linker-loader.png)

文章标题：**介绍一个方便 riscv for aosp 测试的开发环境**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 下载](#1-下载)
- [2. 环境的准备](#2-环境的准备)
- [3. 运行测试或者调试](#3-运行测试或者调试)
	- [3.1. host 方式](#31-host-方式)
	- [3.2. target 方式](#32-target-方式)
		- [3.2.1. Make minimal rootfs for testing](#321-make-minimal-rootfs-for-testing)
		- [3.2.2. Dump the minimal rootfs](#322-dump-the-minimal-rootfs)
		- [3.2.3. Launch the minial system with qemu and run bionic test](#323-launch-the-minial-system-with-qemu-and-run-bionic-test)

<!-- /TOC -->

当初为了方便移植 aosp 到 riscv64 上，第一步主要是移植 bionic 库（类似于 GNU 生态下的 glibc），因为当时缺少硬件和模拟器的支持，所以自己基于 QEMU 和一些开源软件搭建了一个方便测试和调试的开发环境，现在把它整理好后开源出来，希望对大家有所帮助，也算是对我过去一些工作的总结。

这个环境支持集成到 AOSP 的开发环境中使用，具体搭建和使用介绍在这里整理成一篇 blog，以后如果有变化也会在这里更新。

# 1. 下载

首先你需要搭建一个 AOSP 的开发环境，国内的小伙伴建议参考[《官方的 AOSP 构建环境参考》][1]。AOSP 开发环境的搭建不是一般痛苦，特别是对于个人开发者~~~


我这里的系统使用的是 Ubuntu：

```shell
$ lsb_release -a
No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 20.04.5 LTS
Release:	20.04
Codename:	focal
$ uname -a
$ uname -a
Linux u2 5.15.0-69-generic #76~20.04.1-Ubuntu SMP Mon Mar 20 15:54:19 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux
```

AOSP 就用最新的好了，现在截止本文第一次 commit 时间（2023 年 4 月），AOSP 官方主线已经支持了初步的 riscv64，具体参考 [Google 的官方说明][2] 执行 lunch 就好。假设我们下载后的 AOSP 的路径在 `<AOSP>`。

然后在 `<AOSP>` 下找个地方下载我们的测试环境，假设我们选择 `<AOSP>/test`

```shell
$ cd <AOSP>/test
$ git clone git@gitee.com:aosp-riscv/test-riscv.git
```

# 2. 环境的准备

本测试环境已经与 AOSP 开发环境集成，所以遵循 AOSP 的构建要求，首先选择产品：

```shell
$ cd <AOSP>
$ source build/envsetup.sh
$ lunch aosp_cf_riscv64_phone-userdebug
```

继续测试前首先需要设置一下 test-riscv 的环境。首先编辑一下 `./test/test-riscv/envsetup.sh` 这个文件，设置如下环境变量：

- `PATH_QEMU_BIN`: qemu 的可执行程序的路径，推荐使用比较新的版本，譬如 7.2
- `PATH_GNUTOOLS_BIN`: riscv gnu toolchain 的可执行程序的路径，同样推荐使用比较新的版本，我这里使用的是 ["中科院软件所的 RISC-V Toolchains 的国内镜像网站"] 提供的预编译 riscv-gnu-toolchain，下载地址是 <https://mirror.iscas.ac.cn/riscv-toolchains/release/riscv-collab/riscv-gnu-toolchain/LatestRelease/>，我使用的是 `riscv64-glibc-ubuntu-20.04-nightly-2023.03.14-nightly.tar.gz`。
- `PATH_KERNEL`：target 模式下使用的 Linux 内核的 image。这个我默认提供了一个。你也可以使用自己的或者 AOSP 自带的 prebuilt 的版本。
- `PATH_ROOTFS_TEMPLATE`：target 模式下构建自己的 rootfs 的一个模板系统，默认提供了一套我自己用 busybox 做的一个框架，后面运行 `make_rootfs.sh` 脚本会在这个框架上添加来自 aosp 的其他库和文件并生成最终的 `rootfs.img`。

修改好以上环境变量后，执行 source 命令导入该配置文件。

```shell
$ source ./test/test-riscv/envsetup.sh 
```

这个导入的操作只需要执行一次，和 AOSP 中执行 `source build/envsetup.sh` 类似。

# 3. 运行测试或者调试

然后可以开始使用我们的开发环境。以 bionic 的测试开发为例：

有关 bionic 的测试可以阅读 [源码仓库的 README 文档][3] 中的 "Running the tests" 的内容。

正式的 aosp 测试需要完整的 aosp system image 并通过硬件（手机/开发板）或者模拟器支持，但在移植 riscv64 初期，包括直到现在，其实都还没有在这些依赖，特别是硬件上得到很号的支持，所以可以尝试我这里做的这个简易开发环境系统。

本开发系统支持两种方式运行 bionic 测试：

- host 方式：利用 qemu 的 user mode 工具在 linux host 上运行 bionic 甚至 gdb 调试。
- target 方式：自己构造 aosp 的 rootfs 并使用 qemu 的 system mode 工具运行。对于 target 方式，目前最新的 Gogole 上游的 cuttle 已经 ready 并支持 riscv64。具体参考 [Google 的官方说明][2]。所以我原来自己做的这种 target 方式其实已经用处不大了，相关代码保留可以供大家参考。

## 3.1. host 方式

提供了 “运行” 和 “调试” 两套脚本。

运行脚本：

- `<AOSP>/test/test-riscv/bionic/host/run.sh`: run bionic-unit-tests
- `<AOSP>/test/test-riscv/bionic/host/run-static.sh`: run bionic-unit-tests-static
- `<AOSP>/test/test-riscv/bionic/host/run-linker.sh`: run linker directly

举个例子：
```shell
$ ./test/test-riscv/bionic/host/run.sh wctype.wctype_l
========== Start Running ......
Note: Google Test filter = wctype.wctype_l
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from wctype
[ RUN      ] wctype.wctype_l
[       OK ] wctype.wctype_l (1 ms)
[----------] 1 test from wctype (1 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (4 ms total)
[  PASSED  ] 1 test.
```



调试脚本：

- `<AOSP>/test/test-riscv/bionic/host/debug.sh`: debug bionic-unit-tests
- `<AOSP>/test/test-riscv/bionic/host/debug-static.sh`: debug bionic-unit-tests-static
- `<AOSP>/test/test-riscv/bionic/host/debug-linker.sh`: debug linker directly

举个例子：
```shell
$ ./test/test-riscv/bionic/host/debug.sh wctype.wctype_l
========== Start Running ......
GNU gdb (GDB) 12.1
Copyright (C) 2022 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "--host=x86_64-pc-linux-gnu --target=riscv64-unknown-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<https://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from /home/u/ws/aosp/out/target/product/vsoc_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests...
Source directories searched: /home/u/ws/aosp:$cdir:$cwd
Breakpoint 1 at 0x25568e: main. (2 locations)
__dl__start () at bionic/linker/arch/riscv64/begin.S:35
35	  mv a0, sp
=> 0x00000040031647e0 <__dl__start+0>:	0a 85	mv	a0,sp
(gdb) 
```

## 3.2. target 方式

目前 AOSP 的主线上的 cuttlefish 模拟器已经支持运行 riscv64 的系统，所以该方式逐渐失去价值，这里只是在这里记录一下。以下叫脚本执行的 pwd 都是 `<AOSP>`。

### 3.2.1. Make minimal rootfs for testing

```shell
$ ./test/test-riscv/make_rootfs.sh
```
该脚本会生成 `<AOSP>/test/test-riscv/out/rootfs.img`。

### 3.2.2. Dump the minimal rootfs

If you make some changes in your minimal system, for example, you have created testing log and want fetch it out.

```shell
$ sudo -E ./test/test-riscv/dump_rootfs.sh
```

Note, root privilege is required to run this script. 注意需要加上 `-E` 以便保留原会话的环境变量，否则 sudo 默认会另外起一套环境变量导致脚本执行失败。

A folder `<AOSP>/test/test-riscv/out/rootfs_dump/` would be created and it contains all that in your minimal system.

### 3.2.3. Launch the minial system with qemu and run bionic test

Launch qemu system and load rootfs

```shell
$ ./test/test-riscv/run.sh
```

Press Enter to acitivate the console and go to bionic testing folder. We provide scripts for quick launching of bionic tests.

Please press Enter to activate this console. 
```shell
/ # cd tests/bionic/
```

To run static link version for bionic unit tests

```shell
/tests/bionic # ./bionic-unit-tests-static.sh
```

To run dynamic link version for bionic unit tests

```shell
/tests/bionic # ./bionic-unit-tests.sh 
```

To test specific suite/cases, run the script with positive/negative pattern. For example, if you want to test all static bionic unit cases in suite "unistd" but exclude cases "execvpe_ENOEXEC" and "getpid_caching_and_pthread_create" of suite "unistd", you can input as below:

```shell
/tests/bionic # ./bionic-unit-tests-static.sh unistd.*-unistd.execvpe_ENOEXEC:unistd.getpid_caching_and_pthread_create
```

For more details on how to use the bionic-unit-tests-static, run:

```shell
/tests/bionic # /data/nativetest64/bionic-unit-tests-static/bionic-unit-tests-static -h
```

For more details on how to use the bionic-unit-tests, run:

```shell
/tests/bionic # /data/nativetest64/bionic-unit-tests/bionic-unit-tests -h
```


[1]:./20230111-aosp-build.md
[2]:https://github.com/google/android-riscv64/#readme
[3]:https://android.googlesource.com/platform/bionic/+/refs/heads/master/README.md
[4]:https://help.mirrors.cernet.edu.cn/riscv-toolchains/
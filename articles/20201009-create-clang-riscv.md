![](./diagrams/risc-v.png)

文章标题：**制作一个针对 RISC-V 的 LLVM/Clang 编译器**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 参考](#1-参考)
- [2. 准备工作系统环境](#2-准备工作系统环境)
    - [2.1. 硬件](#21-硬件)
    - [2.2. 软件](#22-软件)
- [3. 构造过程](#3-构造过程)
    - [3.1. 下载 llvm 的源码](#31-下载-llvm-的源码)
    - [3.2. 编译前的配置](#32-编译前的配置)
    - [3.3. 执行编译和安装](#33-执行编译和安装)
- [4. 验证一下工具链是否可以工作](#4-验证一下工具链是否可以工作)

<!-- /TOC -->

# 1. 参考

- 【参考 1】：[Getting Started with the LLVM System](https://link.zhihu.com/?target=https%3A//llvm.org/docs/GettingStarted.html)

# 2. 准备工作系统环境

## 2.1. 硬件

我采用的是运行在 x86_64 机器/虚拟机上的 Ubuntu 20.04 LTS。

## 2.2. 软件

本文采用的 LLVM 版本为 14.0.6

根据 【参考 1】，LLVM 官方要求的编译 LLVM 必需的几项主要软件如下：

- CMake：>=3.13.4，用于自动化生成项目编译配置文件，譬如用于 make 的 makefile 或者其他类型的 project 文件
- GCC: >= 7.1.0, LLVM 作为一款应用软件，仍然需要另一款编译器来制作它，这里我们采用 GCC
- python: >=3.6, 仅用于对 LLVM 进行自动化测试
- zlib: >=1.2.3.4, 可选
- GNU Make: 3.79, 3.79.1, 编译自动化驱动软件，LLVM 的编译可以支持多种驱动方式，譬如 Make 或者 Ninja。我们这里使用 Make。

将 Ubuntu 20.04 LTS 更新到最新状态后缺省已经支持以上要求。

其他编译过程中需要的工具，这些基本上 Ubuntu 上都有，如果缺少请自行安装。

# 3. 构造过程

## 3.1. 下载 llvm 的源码

官方源码仓库在 github：<https://github.com/llvm/llvm-project>，国内的用户可以从 gitee 的 mirror <https://gitee.com/mirrors/llvm-project> 下载：

```
$ git clone https://gitee.com/mirrors/llvm-project.git
```

下载后进入源码仓库根目录并检出相应版本，目前最新的正式发布版本是 14.0.6，所以我们选择切换到该版本对应的 tag：llvmorg-14.0.6 上

```
$ cd llvm-project/
$ git checkout llvmorg-14.0.6 -b llvmorg-14.0.6
```
## 3.2. 编译前的配置

编译前需要在 llvm 的源码根目录下新建一个 build 目录，然后进入这个目录进行 make，以前官方文档说 `in-tree build is not supported`， 即不支持在 llvm-project 目录下直接编译，否则会失败，但现在最新的文档上这句话没有了，但我们还是按照老习惯操作吧。这个 build 目录官方的正式定义叫 `OBJ_ROOT`，所有 cmake 生成的项目配置文件，以及编译过程中生成的 `.o` 文件和最终的 bin 文件都会存放在这个目录下，不会污染原来的代码仓库。

```
$ mkdir build
$ cd build
```

使用 cmake 对编译进行配置。LLVM 使用 cmake 来对编译进行配置，摘录一段官网的说明如下：

> Once checked out repository, the LLVM suite source code must be configured before being built. This process uses CMake. Unlinke the normal configure script, CMake generates the build files in whatever format you request as well as various *.inc files, and llvm/include/Config/config.h.

使用 cmake 的基本命令模板如下：

```
$ cd OBJ_ROOT
$ cmake -G <generator> [options] SRC_ROOT
```

其中：

- OBJ_ROOT 即我们这里创建的 build 子目录。
- generator 是个字符串，表示用于驱动编译工具（譬如 gcc）执行编译生成 llvm 的工具的名称，如果有空格需要用双引号括起来，cmake 支持跨平台开发，有以下四种选项：
  - Unix Makefiles: 即采用 Unix 上传统的 Make，指定该选项后 cmake 负责生成用于 Make 的 makefile 文件。
  - Ninja: 采用 Ninja，指定该选项后 cmake 负责生成用于 Ninja 的 build.ninja 文件。这是 LLVM 的开发社区推荐采用的方式，因为对于像 LLVM 这样的大型软件来说，采用 Ninja 会大大加速编译的速度。
  - Visual Studio: 指示 cmake 产生用于 Visual Studio 的项目构造文件。
  - Xcode: 指示 cmake 产生用于 Xcode 的项目构造文件, Xcode 是运行在操作系统 MacOS X 上的集成开发工具（IDE）。

- `SRC_ROOT`: LLVM 的官方定义是 `the top level directory of the LLVM source tree`, 在这里指的就是我们下载的仓库根目录 `llvm-project` 下的 `llvm` 子目录，在 LLVM 项目中 `llvm` 子目录存放的是这个项目的主框架代码，是必须要编译的对象。

- options，以 `-D` 开头定义的选项宏，如果超过一个则用空格分隔。这些选项会影响 cmake 生成的构造配置文件并进而影响整个编译构造过程，针对 LLVM 常用的有以下这些，更多选项请参阅 【参考 1】：

  - `CMAKE_BUILD_TYPE=type`: 指定生成的应用程序（这里当然指的是 LLVM）的类型，type 包括 Debug、Release、RelWithDebInfo 或者 MinSizeRel。如果不指定缺省为 Debug。
  - `CMAKE_INSTALL_PREFIX=directory`: 用于指定编译完后安装 LLVM 工具和库的路径，如果不指定，默认安装在 `/usr/local`。
  - `LLVM_TARGETS_TO_BUILD`: 用于指定生成的 LLVM 可以支持的体系架构（这里称为 target），LLVM 和 GCC 有个很大的不同点是， GCC 需要为每个特定的体系架构，譬如 `arm/x86` 独立生成一套交叉工具链套件，而 LLVM 是在一个工具链套件中就可以支持多个体系架构。如果不指定，默认会编译所有的 targets，具体的 targets 有哪些，可以看源码 `llvm-project/llvm/CMakeLists.txt` 中 `LLVM_ALL_TARGETS` 的定义。具体制作时可以自己指定需要的 targets，通过以分号（semicolon）分隔方式给出，譬如 `-DLLVM_TARGETS_TO_BUILD="ARM;PowerPC;X86"`。
  - `LLVM_DEFAULT_TARGET_TRIPLE`: 可以通过该选项修改默认的 target 的 triple 组合，不指定默认是 `x86_64-unknown-linux-gnu`。
  - `LLVM_ENABLE_PROJECTS='...'`: LLVM 是整个工具链套件的总称，LLVM 下包括了很多个子项目，譬如 clang, clang-tools-extra, libcxx, libcxxabi, libunwind, lldb, compiler-rt, lld, polly, or debuginfo-tests 等。如果不指定该选项，默认只编译 llvm 这个主框架。如果要选择并指定编译哪些子项目，可以通过分号分隔方式给出，譬如我们在编译 llvm 之外还想编译 Clang, libcxx, 和 libcxxabi, 那么可以写成这样：`-DLLVM_ENABLE_PROJECTS="clang;libcxx;libcxxabi"`。

基于以上理解执行如下命令：

```
$ cmake -G Ninja \
-DCMAKE_BUILD_TYPE=Release \
-DCMAKE_INSTALL_PREFIX=../../install \
-DLLVM_TARGETS_TO_BUILD="RISCV" \
-DLLVM_ENABLE_PROJECTS="clang" \
-DLLVM_USE_LINKER=gold \
-DLLVM_DEFAULT_TARGET_TRIPLE="riscv64-unknown-linux-gnu" \
../llvm
```

简单解释一下以上命令的效果就是: 
- 采用 Ninja 方式编译 LLVM，这需要你在 Ubuntu 里提前采用 apt install ninja；
- 编译 Release 版本（我们这里只是使用 llvm 工具链，不涉及开发，所以采用 Release 方式可以缩短编译时间和减少对硬盘的消耗，生成的可执行程序执行速度也快）；
- 编译完成后如果要安装将安装在和 llvm-project 源码树平级的 install 子目录下，确保不会污染 llvm-project 源码目录；
- 只编译 RISCV 的 target；
- 链接时使用 gold 而不是默认的 gnu-ld，加快链接速度，需要你在 Ubuntu 里提前采用 apt install binutils；
- 修改默认的 triple 组合为 `riscv64-unknown-linux-gnu`（避免在后面编译时再通过 `--target` 指定）；除了 llvm 外还会生成 clang。

## 3.3. 执行编译和安装
```
$ ninja -j $(nproc)
$ ninja install
```

简单检查一下安装的结果

```
$ ls ../../install/ -l
total 20
drwxrwxr-x 2 u u 4096 10月  9 11:37 bin
drwxrwxr-x 7 u u 4096 10月  9 11:37 include
drwxrwxr-x 4 u u 4096 10月  9 11:37 lib
drwxrwxr-x 2 u u 4096 10月  9 11:37 libexec
drwxrwxr-x 7 u u 4096 10月  9 11:37 share
```

检查一下生成的 clang 的版本：
```
$ ../install/bin/clang -v
clang version 14.0.6 (https://gitee.com/mirrors/llvm-project.git f28c006a5895fc0e329fe15fead81e37457cb1d1)
Target: riscv64-unknown-linux-gnu
Thread model: posix
InstalledDir: ......
```

为了后面直接在命令行中输入 clang 运行编译器， 将安装 clang 工具所在路径添加到 PATH 环境变量中，这里不再啰嗦。

# 4. 验证一下工具链是否可以工作

编辑一个简单的 `test.c` 文件

```cpp
#include <stdio.h>

int main(int argc, char *argv[])
{
 printf("Hello, world!\n");
 return 0;
}
```

受限于 LLVM 自身的 C 库不完善以及 lld 链接器尚未实现 linker relaxation，所以我们目前在使用 clang 时仍然需要借用 GNU 的 C 库和链接器来生成 RISC-V 的可执行程序（如果是本地 X86 则不用这么麻烦）。

运行 clang 编译程序，通过 `--sysroot` 选项来指定 gnu 工具链的 sysroot，通过 `--gcc-toolchain` 来指定 gcc 工具链的位置。

这里假设使用在 [在 QEMU 上运行 RISC-V 64 位版本的 Linux](https://zhuanlan.zhihu.com/p/258394849) 一文中制作的 GNU GCC。同时假设我们还是利用上文中的环境来测试，由于我们当时制作的文件系统很简单，不包含任何 c 库，所以我们采用静态链接的方式生成可执行程序。

```
$ clang --gcc-toolchain=/opt/riscv64 --sysroot=/opt/riscv64/sysroot/ -static test.c
```

简单检查一下生成的可执行文件格式是否正确：

```
$ file a.out
a.out: ELF 64-bit LSB executable, UCB RISC-V, version 1 (SYSV), statically linked, for GNU/Linux 4.15.0, with debug_info, not stripped
```

的确是满足 64 位 RISC-V。然后利用 qemu 测试生成的 `a.out` ：

```
qemu-riscv64 ./a.out
Hello, world!
```

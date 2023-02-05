![](./diagrams/android.png)

文章标题：**笔记：Clang for Chromium 构建分析**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

最近开始移植 Chrome 到 android for riscv64 的工作。期间发现 Chromium 的项目采用的 clang 是自己维护的，但是还无法支持 riscv64 for android。所以移植工作的第一步就是要尝试为 Chromium Clang 加上对 riscv64 for android 的支持。为此特意研究了一下 Chromium Clang 的构建相关内容，笔记整理如下。
<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 搭建 Chromium for Android 的构建环境](#2-搭建-chromium-for-android-的构建环境)
- [3. 构建 Clang for Chromium](#3-构建-clang-for-chromium)
- [4. 构建脚本分析](#4-构建脚本分析)
	- [4.1. `build.py` 代码分析](#41-buildpy-代码分析)
	- [4.2. `package.py` 代码分析](#42-packagepy-代码分析)
	- [4.3. `update.py` 代码分析](#43-updatepy-代码分析)

<!-- /TOC -->

# 1. 参考文档 

- [1] [Checking out and building Chromium for Android][1]
- [2] [Checking out and building on Fuchsia][2]
- [3] [Clang for Chromium 的总入口][3] 注意这篇文档里给出了 “Related documents”，包括：
```
  - [Toolchain support](toolchain_support.md) gives an overview of clang
  rolls, and documents when to revert clang rolls and how to file good
  toolchain bugs.

  - [Updating clang](updating_clang.md) documents the mechanics of updating clang,
  and which files are included in the default clang package.

  - [Clang Sheriffing](clang_sheriffing.md) contains instructions for how to debug
  compiler bugs, for clang sheriffs.

  - [Clang Tool Refactoring](clang_tool_refactoring.md) has notes on how to build
  and run refactoring tools based on clang's libraries.
```

- [4] [RFC: Time to drop legacy runtime paths?][4]
- [5] [Handling version numbers in per-target runtime directories][5]
- [6] [[CMake] Multi-target builtins build][6]
- [7] [[CMake] Support multi-target runtimes build][7]
- [8] [Compiling cross-toolchains with CMake and runtimes build][8]

注意本文相关代码基于 Chromium 的版本是 `109.0.5414.87`

# 2. 搭建 Chromium for Android 的构建环境

基本上就是参考的 [1]，只是简单将我自己的操作步骤列出来。

我的环境如下，注意 Google 也建议使用 Ubuntu：

```bash
lsb_release -a
LSB Version:	core-11.1.0ubuntu2-noarch:security-11.1.0ubuntu2-noarch
Distributor ID:	Ubuntu
Description:	Ubuntu 20.04.5 LTS
Release:	20.04
Codename:	focal
```

假设我要取一个 `109.0.5414.87` 的 Chromium 的版本，操作步骤如下：

“Install depot_tools” 的步骤省略，直接从 “Get the code” 开始。

```bash
mkdir ~/chromium && cd ~/chromium
fetch --nohooks android
cd src
git checkout 109.0.5414.87 -b dev-109.0.5414.87
build/install-build-deps.sh --android
gclient runhooks
```

注意运行 `install-build-deps.sh` 需要 sudo，其内部实际上就是在执行 `apt install`。

# 3. 构建 Clang for Chromium

注意在构建 Clang for Chromium 时，仅按照上一节搭建的代码环境是不够的，特别我实验发现构建过程中会报和 fuchsia 相关的 sdk 找不到或者相关文件不存在的问题。我目前理解从 Clang for Chromium 的角度来看，其 Clang 要支持构建出来的 Chrome 运行在各种 os 上，包括 linux/android/fuchsia。所以为了顺利地构建上，我们需要把 fuchsia 相关的依赖包也装上。

参考 [1] 和 [2]，在上一节的基础上继续执行如下步骤，添加 Fuchsia 相关的依赖。

编辑 `.gclient` 以添加 `fuchsia` 到 `target_os` 列表中。修改后该文件应如下所示：

```python
solutions = [
  {
    "name": "src",
    "url": "https://chromium.googlesource.com/chromium/src.git",
    "managed": False,
    "custom_deps": {},
    "custom_vars": {},
  },
]
target_os = ["android", "fuchsia"]
```

然后运行 `gclient sync` 命令即可。

注意，似乎 `target_os` 并不需要加上 `"linux"`，暂时不太清楚原因，可能 "android" 本身也是属于 "linux" 的一个分支，所以有了 "android"，至少对构建 Clang for Chromium 是没有问题了。

现在我们可以执行脚本构建了。我们推荐执行 `package.py` 而不是 `build.py`，因为 `package.py` 不仅内部封装了调用 `build.py` 以及相关的调用参数，而且会创建日志文件以及将我们需要的可执行文件和库结果提取出来打包方便我们部署。

假设 pwd 为上面的 `~/chromium/src`

注意，如果整个流程完整正确地结束，脚本会自动删除生成的日志文件，所以如果一定要查看日志文件，需要 hack 一下代码，修改 `~/chromium/src/tools/clang/scripts/package.py`，找到脚本会删除日志的地方，把删除的动作注释掉，例子如下：
```python
  # Upload build log next to it.
  os.rename('buildlog.txt', pdir + '-buildlog.txt')
  MaybeUpload(args.upload,
              pdir + '-buildlog.txt',
              gcs_platform,
              extra_gsutil_args=['-z', 'txt'])
  #os.remove(pdir + '-buildlog.txt')
```

现在可以执行打包和构建的脚本了：

```bash
./tools/clang/scripts/package.py
```

执行完毕后，在 `~/chromium/src` 下会生成许多 `*.tar.xz` 或者 `*.tgz` 文件，如果是同名的只是打包格式不同。
```bash
$ ls *.t*z -l
-rw-rw-r-- 1 wangchen wangchen  9690880 Feb  1 10:53 clangd-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen 14792275 Feb  1 10:53 clangd-llvmorg-16-init-8697-g60809cd2-1.tgz
-rw-rw-r-- 1 wangchen wangchen  1102924 Feb  1 10:53 clang-format-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen  1397694 Feb  1 10:53 clang-format-llvmorg-16-init-8697-g60809cd2-1.tgz
-rw-rw-r-- 1 wangchen wangchen 45790480 Feb  1 10:59 clang-libs-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen 90234059 Feb  1 10:59 clang-libs-llvmorg-16-init-8697-g60809cd2-1.tgz
-rw-rw-r-- 1 wangchen wangchen 44955744 Feb  1 10:52 clang-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen 86565553 Feb  1 10:52 clang-llvmorg-16-init-8697-g60809cd2-1.tgz
-rw-rw-r-- 1 wangchen wangchen 11628060 Feb  1 10:53 clang-tidy-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen 18412479 Feb  1 10:53 clang-tidy-llvmorg-16-init-8697-g60809cd2-1.tgz
-rw-rw-r-- 1 wangchen wangchen    25436 Feb  1 11:00 libclang-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen    29581 Feb  1 11:00 libclang-llvmorg-16-init-8697-g60809cd2-1.tgz
-rw-rw-r-- 1 wangchen wangchen  1862928 Feb  1 10:52 llvm-code-coverage-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen  3590033 Feb  1 10:52 llvm-code-coverage-llvmorg-16-init-8697-g60809cd2-1.tgz
-rw-rw-r-- 1 wangchen wangchen  5233880 Feb  1 10:52 llvmobjdump-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen 14178031 Feb  1 10:52 llvmobjdump-llvmorg-16-init-8697-g60809cd2-1.tgz
-rw-rw-r-- 1 wangchen wangchen  6287948 Feb  1 11:00 translation_unit-llvmorg-16-init-8697-g60809cd2-1.tar.xz
-rw-rw-r-- 1 wangchen wangchen  9545236 Feb  1 11:00 translation_unit-llvmorg-16-init-8697-g60809cd2-1.tgz
```

其中 `clang-llvmorg-*.tar.xz` 或者 `clang-llvmorg-*.tgz` 就是我们要的 Clang 的压缩包，也是我们通过执行 `update.py` 下载的文件。

值得注意的是：我发现将这里打包的 tar 中的文件和我们执行 runhooks （hook 中会调用 `update.py`）后创建的 `<SRC>/third_party/llvm-build/Release+Asserts/` 下的内容进行比较，发现 tar 中的文件会少一些。列举如下：

```
Only in ../chromium/src/third_party/llvm-build.bak/Release+Asserts/bin/: llvm-bcanalyzer
Only in ../chromium/src/third_party/llvm-build.bak/Release+Asserts/bin/: llvm-cxxfilt
Only in ../chromium/src/third_party/llvm-build.bak/Release+Asserts/bin/: llvm-dwarfdump
Only in ../chromium/src/third_party/llvm-build.bak/Release+Asserts/bin/: llvm-objdump
Only in ../chromium/src/third_party/llvm-build.bak/Release+Asserts/bin/: llvm-otool
```

我理解可能 hooks 中还在 Release+Asserts 下多创建了这些文件，具体有什么用，需要再看看，目前至少要理解我们 build 出来的 clang 的 tar 包中和 runhooks 后的 "Release+Asserts" 下的文件内容并不是完全一致的。这个后面可能需要注意一下。

# 4. 构建脚本分析

Chromium 构建 Clang 的脚本是用 python 写的，主要有以下三个比较重要的：

假设 `<SRC>` 代表上面的 `~/chromium/src`。

- `<SRC>/tools/clang/scripts/build.py`: 构建 Clang 的核心脚本
- `<SRC>/tools/clang/scripts/package.py`: 内部会调用 `build.py` 构建 Clang，然后将其打包成 tar，如果指定了上传选项还会将打包的结果，也就是 prebuilt 的 clang 上传到 Google 的文件服务器。
- `<SRC>/tools/clang/scripts/update.py`: 这个脚本的主要功能就是从 Google 的文件服务器上下载 prebuilt 的 clang。

以下对这些代码的分析基于 `109.0.5414.87` 的 Chromium 的版本。分析内容以 `###` 开头的注释方式嵌入。

## 4.1. `build.py` 代码分析


脚本执行过程中会涉及以下目录, 为方便理解，这里根据我的实验环境给出实际的展开形式

- `THIRD_PARTY_DIR`            = `src/third_party`
- `LLVM_DIR`                   = `src/third_party/llvm`
- `COMPILER_RT_DIR`            = `src/third_party/llvm/compiler-rt`
- `LLVM_BOOTSTRAP_DIR`         = `src/third_party/llvm-bootstrap`
- `LLVM_BOOTSTRAP_INSTALL_DIR` = `src/third_party/llvm-bootstrap-install`
- `LLVM_INSTRUMENTED_DIR`      = `src/third_party/llvm-instrumented`
- `LLVM_PROFDATA_FILE`         = `src/third_party/llvm-instrumented/profdata.prof`
- `LLVM_BUILD_TOOLS_DIR`       = `src/third_party/llvm/../llvm-build-tools` = `src/third_party/llvm-build-tools` 这是一个 build.py 执行过程中创建的目录，用来存放 build 过程中下载下来的一些用来辅助构建的工具，譬如：
  ```bash
  $ ls third_party/llvm-build-tools -l
  total 32
  drwxrwxr-x  6 wangchen wangchen 4096 Jan 30 21:13 cmake-3.23.0-linux-x86_64
  drwxr-xr-x  8 wangchen wangchen 4096 Feb 25  2020 debian_bullseye_amd64_sysroot
  drwxr-xr-x  7 wangchen wangchen 4096 Feb 25  2020 debian_bullseye_arm64_sysroot
  drwxr-xr-x  7 wangchen wangchen 4096 Feb 25  2020 debian_bullseye_arm_sysroot
  drwxr-xr-x  7 wangchen wangchen 4096 Feb 25  2020 debian_bullseye_i386_sysroot
  drwxrwxr-x  9 wangchen wangchen 4096 Feb  3 08:46 gcc-10.2.0-bionic
  drwxrwxr-x 18 wangchen wangchen 4096 Feb  3 08:47 libxml2-v2.9.12
  drwxrwxr-x  4 wangchen wangchen 4096 Jan 31 16:28 pinned-clang
  ```
- `ANDROID_NDK_DIR`            = `src/third_party/android_ndk`
- `FUCHSIA_SDK_DIR`            = `src/third_party/fuchsia-sdk/sdk`
- `PINNED_CLANG_DIR`           = `src/third_party/llvm-build-tools/pinned-clang`

重点分析其 `main()` 函数。

```python
def main():
  ### parse 命令行参数
  ......

  ### 对 parse 后的命令参数进行分析和初步的处理
  ### 其中比较重要的动作包括:
  ### 如果没有指定 --skip-checkout，这也是常规情况下会 clone llvm 的源码到 LLVM_DIR 
  ### 并 checkout 到特定的版本，默认为 CLANG_REVISION，以上关注 CheckoutLLVM 这个函数
  ### 在这个阶段的最后会打印 "Locally building clang <PACKAGE_VERSION>"
  ### 并创建一个空的 STAMP_FILE，默认指向 src/third_party/llvm-build/Release+Asserts/cr_build_revision
  ### 并创建一个空的 FORCE_HEAD_REVISION_FILE，默认指向 src/third_party/llvm-build/force_head_revision
  ......

  ### 下载一个 cmake, from
  ### https://commondatastorage.googleapis.com/chromium-browser-clang/tools/cmake-3.23.0-linux-x86_64.tar.gz
  ### 可见 google 维护了一个公共仓库，将一些 prebuilt 的软件放在这里下载后解压在
  ### src/third_party/llvm-build-tools/cmake-3.23.0-linux-x86_64/
  ### 也就是 <LLVM_BUILD_TOOLS_DIR>/cmake-3.23.0-linux-x86_64/
  AddCMakeToPath(args)

  ### 这里最后一个机会可以跳过后面的构建过程
  if args.skip_build:
    return 0

  ### 开始设置 **基本的** cmake 的 configurations, 保存在 base_cmake_args 中
  ### 之所以叫 **基本的**，是因为整个构建过程会执行好几遍编译（具体描述见下），每一遍编译的配
  ### 置都会在这个 base_cmake_args 上进行相应的修改，而 base_cmake_args 定义了这几次编译中
  ### 相对 common 的部分。
  ### common 部分包含如下关键的配置:
  ### - cc/cxx: 编译 clang 的 c/c++ 编译器，可以指定使用 gcc（`--gcc-toolchain`），不指定
  ###   则会下载一个 chrome team 预先做好的 clang，称之为 "Pinned Clang", 放在 PINNED_CLANG_DIR 下
  ### - lld: 只有在 Windows 上编译 clang 过程中才会涉及需要显式指定链接器，其他情况默认会在调
  ###   用 gcc 或者 "Pinned Clang"（即所谓的 compiler driver）时通过传入 "-fuse-ld=lld"
  ###   来指定链接器
  ### - cflags
  ### - cxxflags
  ### - ldflags
  ### - targets = 'AArch64;ARM;Mips;PowerPC;RISCV;SystemZ;WebAssembly;X86'
  ###   这里是 chromium 支持的 tareget platform 的 ARCH。
  ### - 如果采用 "Pinned Clang"，还会下载一个 prebuilt 的 gcc-10.2.0-bionic，主要是会用
  ###   到其中的 gcc 的 libstdc++，具体谁会用到，TBD
  ### - 如果当前构建系统是 linux，则下载构建中需要的 sysroot，目前支持四个 amd64/i386/arm/arm64。
  ###   我理解目前能实际跑 clang 的 target 平台也就 amd64 和 arm64，i386 和 arm 的用处 TBD
  ###   所以下载 sysroot 后会设置 CMAKE_SYSROOT，默认采用 amd64
  ### - 其他，譬如 LibXml2 ......
  base_cmake_args = [
  ......

  ### bootstrap 构建
  ### 如果命令行选项指定了 `--bootstrap`，则整个构建会做两遍
  ### 以使用 PINNED Clang 为例，第一遍先用 PINNED Clang 构建一个 BOOTSTRAP 的 Clang
  ### 然后第二遍再用生成的 BOOTSTRAP Clang 构建最终的 FINAL Clang
  ### BOOTSTRAP Clang 的 build 目录在 LLVM_BOOTSTRAP_DIR
  ### BOOTSTRAP Clang 的 install 目录在 LLVM_BOOTSTRAP_INSTALL_DIR
  ### BOOTSTRAP Clang 的 cmake configuration 会重新设置，在 base_cmake_args 上进行
  ### 修改，保存在 bootstrap_args 中，注意后加的配置选项如果和 base_cmake_args 重名的会
  ### 覆盖原有定义而不是追加
  ### 直接运行 package.py 得到的 log 显示 bootstrap 的 cmake 配置如下：
  ### cmake -GNinja
  ### -DCMAKE_BUILD_TYPE=Release
  ### -DLLVM_ENABLE_ASSERTIONS=OFF
  ### '-DLLVM_ENABLE_PROJECTS=clang;lld;clang-tools-extra'
  ### -DLLVM_ENABLE_RUNTIMES=compiler-rt
  ### '-DLLVM_TARGETS_TO_BUILD=AArch64;ARM;Mips;PowerPC;RISCV;SystemZ;WebAssembly;X86'
  ### -DLLVM_ENABLE_PIC=ON
  ### -DLLVM_ENABLE_UNWIND_TABLES=OFF
  ### -DLLVM_ENABLE_TERMINFO=OFF
  ### -DLLVM_ENABLE_Z3_SOLVER=OFF
  ### -DCLANG_PLUGIN_SUPPORT=OFF
  ### -DCLANG_ENABLE_STATIC_ANALYZER=OFF
  ### -DCLANG_ENABLE_ARCMT=OFF
  ### '-DBUG_REPORT_URL=https://crbug.com and run tools/clang/scripts/process_crashreports.py (only works inside Google) which will upload a report'
  ### -DLLVM_INCLUDE_GO_TESTS=OFF
  ### -DLLVM_ENABLE_DIA_SDK=OFF
  ### -DLLVM_ENABLE_LLD=ON
  ### -DLLVM_ENABLE_PER_TARGET_RUNTIME_DIR=OFF
  ### -DLLVM_ENABLE_CURL=OFF
  ### -DLIBCLANG_BUILD_STATIC=ON
  ### -DLLVM_STATIC_LINK_CXX_STDLIB=ON
  ### -DCMAKE_SYSROOT=/aosp/wangchen/dev-chrome/chromium/src/third_party/llvm-build-tools/debian_bullseye_amd64_sysroot
  ### -DLLVM_ENABLE_LIBXML2=FORCE_ON
  ### -DLIBXML2_INCLUDE_DIR=/aosp/wangchen/dev-chrome/chromium/src/third_party/llvm-build-tools/libxml2-v2.9.12/build/install/include/libxml2
  ### -DLIBXML2_LIBRARIES=/aosp/wangchen/dev-chrome/chromium/src/third_party/llvm-build-tools/libxml2-v2.9.12/build/install/lib/libxml2.a
  ### -DLLVM_TARGETS_TO_BUILD=X86
  ### '-DLLVM_ENABLE_PROJECTS=clang;lld'
  ### -DLLVM_ENABLE_RUNTIMES=compiler-rt
  ### -DCMAKE_INSTALL_PREFIX=/aosp/wangchen/dev-chrome/chromium/src/third_party/llvm-bootstrap-install
  ### '-DCMAKE_C_FLAGS=-DSANITIZER_OVERRIDE_INTERCEPTORS -I/aosp/wangchen/dev-chrome/chromium/src/tools/clang/scripts/sanitizers -DLIBXML_STATIC'
  ### '-DCMAKE_CXX_FLAGS=-DSANITIZER_OVERRIDE_INTERCEPTORS -I/aosp/wangchen/dev-chrome/chromium/src/tools/clang/scripts/sanitizers -DLIBXML_STATIC'
  ### -DCMAKE_EXE_LINKER_FLAGS=
  ### -DCMAKE_SHARED_LINKER_FLAGS=
  ### -DCMAKE_MODULE_LINKER_FLAGS=
  ### -DLLVM_ENABLE_ASSERTIONS=ON
  ### -DCOMPILER_RT_BUILD_CRT=ON
  ### -DCOMPILER_RT_BUILD_LIBFUZZER=OFF
  ### -DCOMPILER_RT_BUILD_MEMPROF=OFF
  ### -DCOMPILER_RT_BUILD_ORC=OFF
  ### -DCOMPILER_RT_BUILD_PROFILE=ON
  ### -DCOMPILER_RT_BUILD_SANITIZERS=OFF
  ### -DCOMPILER_RT_BUILD_XRAY=OFF
  ### '-DCOMPILER_RT_SANITIZERS_TO_BUILD=asan;dfsan;msan;hwasan;tsan;cfi'
  ### -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON
  ### -DCMAKE_C_COMPILER=/aosp/wangchen/dev-chrome/chromium/src/third_party/llvm-build-tools/pinned-clang/bin/clang
  ### -DCMAKE_CXX_COMPILER=/aosp/wangchen/dev-chrome/chromium/src/third_party/llvm-build-tools/pinned-clang/bin/clang++
  ### /aosp/wangchen/dev-chrome/chromium/src/third_party/llvm/llvm
  ### 所以我们会发现譬如 BOOTSTRAP Clang 只会针对 x86 一个 tareget，enabled project 也
  ### 只有 clang 和 lld；runtime 会构建，但只会生成 crt/builtin/profile 几个有限的。
  ### 注意构建成功后会修改 cc 和 cxx 为 LLVM_BOOTSTRAP_INSTALL_DIR 下的 clang/clang++ 
  ### 这就是为第二阶段做准备了
  if args.bootstrap:
    ........
	RunCommand(['cmake'] + bootstrap_args + [os.path.join(LLVM_DIR, 'llvm')],
               msvc_arch='x64')
    RunCommand(['ninja'], msvc_arch='x64')
    if args.run_tests:
      RunCommand(['ninja', 'check-all'], msvc_arch='x64')
    RunCommand(['ninja', 'install'], msvc_arch='x64')
	......
    print('Bootstrap compiler installed.')

  ### 开始构建 FINAL Clang 
  ### 如果命令行中指定了 "--pgo"，会在构建 FINAL Clang 之前先构建一个所谓的 INSTRUMENTED Clang
  ### 这个 INSTRUMENTED Clang 完全是为了 PGO 的 training 所使用，还不是最终的 FINAL Clang
  ### INSTRUMENTED Clang 的 build 目录在 LLVM_INSTRUMENTED_DIR
  ### INSTRUMENTED Clang 不涉及 install
  ### 直接运行 package.py 得到的 log 显示针对 INSTRUMENTED Clang 的 cmake 配置如下：
  ### 在 base_cmake_args 的基础上覆盖或者追加如下配置
  ### -DLLVM_ENABLE_PROJECTS=clang
  ### '-DCMAKE_C_FLAGS=-DSANITIZER_OVERRIDE_INTERCEPTORS -I/aosp/wangchen/dev-chrome/chromium/src/tools/clang/scripts/sanitizers -DLIBXML_STATIC'
  ### '-DCMAKE_CXX_FLAGS=-DSANITIZER_OVERRIDE_INTERCEPTORS -I/aosp/wangchen/dev-chrome/chromium/src/tools/clang/scripts/sanitizers -DLIBXML_STATIC'
  ### -DCMAKE_EXE_LINKER_FLAGS=
  ### -DCMAKE_SHARED_LINKER_FLAGS=
  ### -DCMAKE_MODULE_LINKER_FLAGS=
  ### -DLLVM_BUILD_INSTRUMENTED=IR
  ### -DCMAKE_C_COMPILER=/aosp/wangchen/dev-chrome/chromium/src/third_party/llvm-bootstrap-install/bin/clang
  ### -DCMAKE_CXX_COMPILER=/aosp/wangchen/dev-chrome/chromium/src/third_party/llvm-bootstrap-install/bin/clang++
  ### 这些配置选项中最关键的就是 DLLVM_BUILD_INSTRUMENTED，这个指定了本次编译的特殊处理，
  ### 编译过程和结果只会构建 X86 的 clang，不会有 builtin 和 runtime
  if args.pgo:
    print('Building instrumented compiler')
    ......
    print('Instrumented compiler built.')

    ### 执行 train，生成的结果就是 LLVM_PROFDATA_FILE，
	### 该文件会在构建 FINAL clang 时作为 `-DLLVM_PROFDATA_FILE=` 的值
    print('Profile generated.')

  ### 一些和 deployment 有关的操作 TBD
  ......

  print('Building final compiler.')

  ### 开始构建 FINAL Clang
  ### 
  ### FINAL Clang 的 build 目录是 LLVM_BUILD_DIR，chdir 的操作在后面 RunCommand 的时候做的
  ### FINAL Clang 的 install 目录也是 LLVM_BUILD_DIR
  ### FINAL Clang 的 cmake configuration 会重新设置，在 base_cmake_args 上进行
  ### 修改，保存在 cmake_args 中
  ### cmake_args 的设置按照比较大的几块，可以认为分好几个部分，简单总结如下
  ### （1）基础部分: 主要是重新设置了 cflags/cxxflags/ldflags 等。值得注意的是这里添加了
  ###      LLVM_EXTERNAL_PROJECTS/LLVM_EXTERNAL_CHROMETOOLS_SOURCE_DIR，这应该就是
  ###      随 clang 一起构建的专门针对 chrome 的一些 plugin
  ......
  ### （2）设置缺省的 LLVM_DEFAULT_TARGET_TRIPLE，这里主要是根据执行构建的主机系统设置。
  ###      其中我们关注的是针对 linux 系统，支持了 aarch64/riscv64/x86_64
  ......
  ### （3）为 cross-building builtins/runtimes 做设置。这里使用了 multi-target 的 build
  ###      方式，即定义 LLVM_BUILTIN_TARGETS/DLLVM_RUNTIME_TARGETS。具体见 “参考6/7/8”
  ###     代码实现是先构建一个数组 runtimes_triples_args，这个数组的每个成员形如：
  ###     (triple, list of CMake vars without '-D').
  ###     举一个成员的例子：('i386-unknown-linux-gnu', ['CMAKE_SYSROOT=xxx', 'LLVM_INCLUDE_TESTS=OFF'])
  ###     这个 runtimes_triples_args 数组会在第（5）步被进一步解析
  ......
  ### （4）Embed MLGO inliner model 处理，因为在 linux 系统上构建时其默认值为 default，所
  ###     以如果我们没有特殊指定这里会从 chrome 网站上下载一个 an official model which
  ###     was trained for Chrome on Android
  if args.with_ml_inliner_model:
	......
    print('Embedding MLGO inliner model at %s using Tensorflow at %s' %
          (model_path, tf_path))
    cmake_args += [
        '-DLLVM_INLINER_MODEL_PATH=%s' % model_path,
        '-DTENSORFLOW_AOT_PATH=%s' % tf_path,
        # Disable Regalloc model generation since it is unused
        '-DLLVM_RAEVICT_MODEL_PATH=none'
    ]
  ### (5) 对第（4）步的结果 runtimes_triples_args 做进一步的处理，
  ### 从代码中我们可以了解到该版本的 Chromium 一共支持以下这些 triples，构建 Clang for 
  ### Chromium 时需要针对这些 triple 制作对应的 runtime/builtin 库文件，并安装到 Clang 
  ### 工具的各自对应的目录下去
  ### GNU Linux:
  ### - i386-unknown-linux-gnu        : lib/clang/16.0.0/lib/i386-unknown-linux-gnu/
  ### - x86_64-unknown-linux-gnu      : lib/clang/16.0.0/lib/x86_64-unknown-linux-gnu/
  ### - armv7-unknown-linux-gnueabihf : lib/clang/16.0.0/lib/armv7-unknown-linux-gnueabihf/
  ### - aarch64-unknown-linux-gnu     : lib/clang/16.0.0/lib/aarch64-unknown-linux-gnu/
  ### Android
  ### - aarch64-linux-android21 : lib/clang/16.0.0/lib/linux/libclang_rt.*-aarch64-android.[so|a]
  ### - armv7-linux-android19   : lib/clang/16.0.0/lib/linux/libclang_rt.*-arm-android.[so|a]
  ### - i686-linux-android19    : lib/clang/16.0.0/lib/linux/libclang_rt.*-i686-android.[so|a]
  ### - x86_64-linux-android21  : lib/clang/16.0.0/lib/linux/libclang_rt.*-x86_64-android.[so|a]
  ### Fuchsia
  ### - aarch64-unknown-fuchsia : lib/clang/16.0.0/lib/aarch64-unknown-fuchsia/
  ### - x86_64-unknown-fuchsia  : lib/clang/16.0.0/lib/x86_64-unknown-fuchsia/
  ### 需要注意的是针对 GNU/Fuchsia 和 Android，runtime 库的存放路径规则并不一样。具体见
  ### “参考 4” 和 “参考 5”，原因是针对这些不同的 os environment，构建是采用了不同的
  ### LLVM_ENABLE_PER_TARGET_RUNTIME_DIR 设定。
  ### 具体 cmake 的参数语法参考以下注释，有点不太明白的是，为何（3）和（5）之间要插入一个（4），
  ### 或者为啥不把（3）挪到（4）后面和（5）一起？
  # Convert FOO=BAR CMake flags per triple into
  # -DBUILTINS_$triple_FOO=BAR/-DRUNTIMES_$triple_FOO=BAR and build up
  # -DLLVM_BUILTIN_TARGETS/-DLLVM_RUNTIME_TARGETS.
  ......
  ### 此时可以对 Final Clang 执行真正的 cmake 和 ninja
  RunCommand(['cmake'] + cmake_args + [os.path.join(LLVM_DIR, 'llvm')],
             msvc_arch='x64',
             env=deployment_env)
  CopyLibstdcpp(args, LLVM_BUILD_DIR)
  RunCommand(['ninja'], msvc_arch='x64')
  ### 如果构建了 chrome 的 tools（plugin），还会安装 plugin
  if chrome_tools:
    # If any Chromium tools were built, install those now.
    RunCommand(['ninja', 'cr-install'], msvc_arch='x64')  
  ......
  ### 运行测试
  # Run tests.  
  if (not args.build_mac_arm and
      (args.run_tests or args.llvm_force_head_revision)):
    RunCommand(['ninja', '-C', LLVM_BUILD_DIR, 'cr-check-all'], msvc_arch='x64')

  if not args.build_mac_arm and args.run_tests:
	......
    RunCommand(['ninja', '-C', LLVM_BUILD_DIR, 'check-all'],
               env=env,
               msvc_arch='x64')

  ### 最后的收尾动作
  WriteStampFile(PACKAGE_VERSION, STAMP_FILE)
  WriteStampFile(PACKAGE_VERSION, FORCE_HEAD_REVISION_FILE)
  print('Clang build was successful.')
  return 0
```

## 4.2. `package.py` 代码分析

重点分析其 `main()` 函数。

```python
def main():
  ### 接续命令行参数，package 的参数很简单，就两个
  ### --upload： 指定是否要上传打包结果
  ### --build-mac-arm：指定是否要在 macOS 上构建 arm 的 clang
  ......

  ### 对我实验的版本，这个 pdir 的字符串内容是：`clang-llvmorg-16-init-8697-g60809cd2-1`
  ### 最后会在 `<SRC>` 下生成 `<pdr>.tar.xz` 和 `<pdr>.tgz` 两个 tar 包，内容一样，只是
  ### 压缩格式不同。
  expected_stamp = PACKAGE_VERSION
  pdir = 'clang-' + expected_stamp
  print(pdir)

  ......

  ### 调用 build.py 构建 clang，同时会生成日志文件 buildlog.txt 在当前目录下，譬如 src
  ### 运行 python3 tools/clang/scripts/package.py ，打印了一下 build_cmd 如下
  ### ['/usr/bin/python3', 'tools/clang/scripts/build.py', '--bootstrap', '--disable-asserts', '--run-tests', '--pgo', '--thinlto']
  ### 构建的结果输出在 LLVM_RELEASE_DIR，打包的时候我们会从中选择一些出来打包
  stamp = open(STAMP_FILE).read().rstrip()
  if stamp != expected_stamp:
    print('Actual stamp (%s) != expected stamp (%s).' % (stamp, expected_stamp))
    return 1

  shutil.rmtree(pdir, ignore_errors=True)

  ### 开始为创建 tar 文件做准备。
  ### 先根据不同的 platform/os 将需要 tar 的文件路径保存在 want 变量中
  ### 注意路径中存在一个 '$V', 
  # Copy a list of files to the directory we're going to tar up.
  # This supports the same patterns that the fnmatch module understands.
  # '$V' is replaced by RELEASE_VERSION further down.
  ......

  ### 将我们需要的文件从存放构建的结果目录 `LLVM_RELEASE_DIR` 中复制到 <pdir> 中
  ### 首先检查我们需要的文件（在 want 中）在 `LLVM_RELEASE_DIR` 中是否存在
  want = [w.replace('$V', RELEASE_VERSION) for w in want]
  found_all_wanted_files = True
  for w in want:
    ......
  ### 再根据 want 将需要的文件挑出来 copy 到 <pdir> 中。
  # TODO(thakis): Try walking over want and copying the files in there instead
  # of walking the directory and doing fnmatch() against want.
  for root, dirs, files in os.walk(LLVM_RELEASE_DIR):
	......
  ### 在 <pdir> 中 创建符号链接
  # Set up symlinks.
  ......

  ### 把 <pdir> 这个目录压缩成 `<pdir>.tar.xz` 和 `<pdir>.tgz` 然后尝试 upload
  # Create main archive.
  ......

  ### 把 `buildlog.txt` 重命名为 `<pdir>-buildlog.txt`，然后尝试 upload，上传后会删掉这个重命名的文件
  ### 注意一旦做过 package，无论是否上传，会把处理过程中的 `buildlog.txt` 给删掉，不知道这个
  ### 是设计如此，还是一个 bug？TBD
  # Upload build log next to it.
  ......

  ### 接下来时将一些其他的东西单独打包
  # Zip up llvm-code-coverage for code coverage.
  ......
  # Zip up llvm-objdump and related tools for sanitizer coverage and Supersize.
  ......
  # Zip up clang-tidy for users who opt into it, and Tricium.
  ......
  # Zip up clangd for users who opt into it.
  ......
  # Zip up clang-format so we can update it (separately from the clang roll).
  ......
  # Zip up clang-libs for users who opt into it. We want Clang and LLVM headers
  # and libs, as well as a couple binaries. The LLVM parts are needed by the
  # Rust build.
  ......
  if sys.platform == 'darwin':
    # dsymutil isn't part of the main zip, and it gets periodically
    # deployed to CIPD (manually, not as part of clang rolls) for use in the
    # Mac build toolchain.
    ......
  # Zip up the translation_unit tool.
  ......
  # Zip up the libclang binaries.
  ......
  if sys.platform == 'win32' and args.upload:
	......
    print('symbol upload took', end - start, 'seconds')

  # FIXME: Warn if the file already exists on the server.
```

## 4.3. `update.py` 代码分析

update 的处理比较简单，可以重点看一下文件的注释部分，摘录如下：

```python
"""This script is used to download prebuilt clang binaries. It runs as a
"gclient hook" in Chromium checkouts.

It can also be run stand-alone as a convenient way of installing a well-tested
near-tip-of-tree clang version:

  $ curl -s https://raw.githubusercontent.com/chromium/chromium/main/tools/clang/scripts/update.py | python3 - --output-dir=/tmp/clang

(Note that the output dir may be deleted and re-created if it exists.)
"""
```

这个脚本的主要功能就是下载 prebuilt 的 clang，在我们执行 `gclient runhooks` 时该脚本也会被调用，从 Google 文件服务器上下载和当前 checkout 的 Chromium 匹配的 prebuilt clang，安装在 `<SRC>/third_party/llvm-build/Release+Asserts` 下面。

当然我们也可以单独运行这个脚本下载 Chromium team 提供的最新的 prebuilt clang 程序。

该脚本的核心处理逻辑在 `UpdatePackage()` 函数中。

- `package_name`: 就是我们要更新的对象，默认为 clang
- `host_os`：构建的主机，默认为 linux
- `LLVM_BUILD_DIR`： 缺省就是 `<SRC>/third_party/llvm-build/Release+Asserts`
- `target_os`：构建的对象所要服务的目标，只有当 `package_name` 为 clang 时才需要，也就是 clang 要构建的可执行程序会运行在什么系统上，从 '.gclient' 文件中读出来，我们这里是 android

- `STAMP_FILE`: 缺省就是 `<SRC>/third_party/llvm-build/Release+Asserts/cr_build_revision` 这个文件, 这个文件之所以叫 stamp，用处是为了避免不必要的更新，具体做法如下：

以我取的 `109.0.5414.87` 这个版本对应的这个文件的内容是 “llvmorg-16-init-8697-g60809cd2-1,android”，也就是包含了两项内容。一个是 `PACKAGE_VERSION`，一个是 `target_os`。记录了当前 `<SRC>/third_party/llvm-build/Release+Asserts/` 下 clang 的 版本信息和 `target_os` 的信息，如果和我们要更新的版本是一致的则什么也不会动作就退出了。
```python
expected_stamp = ','.join([PACKAGE_VERSION] + target_os)
  if ReadStampFile(stamp_file) == expected_stamp:
    return 0
```







[1]:https://chromium.googlesource.com/chromium/src.git/+/HEAD/docs/android_build_instructions.md
[2]:https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/clang.md
[3]:https://chromium.googlesource.com/chromium/src/+/main/docs/fuchsia/build_instructions.md
[4]:https://discourse.llvm.org/t/rfc-time-to-drop-legacy-runtime-paths/64628
[5]:https://discourse.llvm.org/t/handling-version-numbers-in-per-target-runtime-directories/62717
[6]:https://reviews.llvm.org/D26652
[7]:https://reviews.llvm.org/D32816
[8]:https://llvm.org/devmtg/2017-10/slides/Hosek-Compiling%20cross-toolchains%20with%20CMake%20and%20runtimes%20build.pdf
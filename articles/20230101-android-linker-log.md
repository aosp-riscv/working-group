![](./diagrams/android-riscv.png)

文章标题：**Android Dynamic Linker 的日志分析**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>


<!-- TOC -->

- [1. 参考资料](#1-参考资料)
- [2. 控制 linker 的日志的输出方向](#2-控制-linker-的日志的输出方向)
- [3. 控制 linker 的日志的输出级别](#3-控制-linker-的日志的输出级别)
- [4. 看一个实际的 linker 执行的例子](#4-看一个实际的-linker-执行的例子)

<!-- /TOC -->

Android 的 bionic 仓库中实现了 Android 系统的 Dynamic linker（bionic 中也简称 linker，本文中如果提及 linker 也指 dynamic linker）。

linker 的具体代码在 `<AOSP>/bionic/linker` 下。下文以 `<AOSP>` 指代 aosp 采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

android linker, 即 Android 系统上的 dynamic linker，内核在加载动态链接的程序时会先将执行权交给这个本质上也是动态库 so 的组件，由其完成程序依赖的其他 so 的加载和符号重定位，在完成以上工作后，dynamic linker 又会将处理器的执行交还给程序的 main。

在为 Android 移植 riscv 的过程中，这个 dynamic linker 也是移植的一个工作重点，但针对这类基础库软件想要采用 gdb 进行调试不是说不可以，但具体操作起来会比较麻烦，最好用的还是通过 log 日志分析程序的运行。本文主要总结了如何控制 linker 的日志打印，以及结合一个例子，看一下 linker 的主要执行流程。

# 1. 参考资料
- [android bionic linker debug enable](https://blog.csdn.net/green369258/article/details/76691731)
- [Android基础：linker调试技巧](https://www.jianshu.com/p/65828d8c9f9c)
- https://stackoverflow.com/questions/7177802/how-to-enable-the-debug-output-in-the-dynamic-linker-on-android

# 2. 控制 linker 的日志的输出方向

linker 的调试 log 可以输出到两个方向，参考相关的代码：`<AOSP>/bionic/linker/linker_debug.h`
```cpp
// You can increase the verbosity of debug traces by defining the LD_DEBUG
// environment variable to a numeric value from 0 to 2 (corresponding to
// INFO, TRACE, and DEBUG calls in the source). This will only
// affect new processes being launched.

// By default, traces are sent to logcat, with the "linker" tag. You can
// change this to go to stdout instead by setting the definition of
// LINKER_DEBUG_TO_LOG to 0.
#define LINKER_DEBUG_TO_LOG  1
```

以及 `<AOSP>/bionic/linker/linker_debug.cpp`

```cpp
void linker_log_va_list(int prio __unused, const char* fmt, va_list ap) {
#if LINKER_DEBUG_TO_LOG
  async_safe_format_log_va_list(5 - prio, "linker", fmt, ap);
#else
  async_safe_format_fd_va_list(STDOUT_FILENO, fmt, ap);
  write(STDOUT_FILENO, "\n", 1);
#endif
}
```

可见，默认情况下，`LINKER_DEBUG_TO_LOG` 为非零值，则输出到 logcat, 否则输出到 stdout。

注意以上描述是指在 target 环境下运行，如果我们是在 host 上采用 qemu 方式直接加载程序运行，则 log 会直接输出在 host 的 stdio 上，无需修改代码更改 `LINKER_DEBUG_TO_LOG` 为 0。

# 3. 控制 linker 的日志的输出级别

同时从上面注释中我们还知道 Linker 的 log 级别分为三个档，相关的代码参考 `<AOSP>/bionic/linker/linker_debug.h`

```cpp
#define LINKER_VERBOSITY_PRINT (-1)
#define LINKER_VERBOSITY_INFO   0
#define LINKER_VERBOSITY_TRACE  1
#define LINKER_VERBOSITY_DEBUG  2
```

对应代码中的三个 log 函数调用
```cpp
#define _PRINTVF(v, x...) \
    do { \
      if (g_ld_debug_verbosity > (v)) linker_log((v), x); \
    } while (0)

#define PRINT(x...)          _PRINTVF(LINKER_VERBOSITY_PRINT, x)
#define INFO(x...)           _PRINTVF(LINKER_VERBOSITY_INFO, x)
#define TRACE(x...)          _PRINTVF(LINKER_VERBOSITY_TRACE, x)
```

`g_ld_debug_verbosity` 的值默认为 0，但我们可以利用 `LD_DEBUG` 这个环境变量在运行时改变其默认值。这个我们可以参考 `linker_main()` 这个函数中的以下处理：
```cpp
  // Get a few environment variables.
  const char* LD_DEBUG = getenv("LD_DEBUG");
  if (LD_DEBUG != nullptr) {
    g_ld_debug_verbosity = atoi(LD_DEBUG);
  }
```

综上可知，我们可以按如下方式设置 `LD_DEBUG` 这个环境变量：

- 3 : 最高级别，打印最丰富，所有的 log，包括 debug 级别的 log 也会输出，但输出太多，要注意控制使用。
- 2 : 打印直到 trace 级别
- 1 : 打印直到 info 级别，相当于 Warning
- 0 : 默认级别，相当于只有 Fatal 的 log 才会输出

我们可以尝试如下命令看看效果：

```bash
$ qemu-riscv64 -E LD_DEBUG=0 ./out/target/product/generic_riscv64/system/bin/bootstrap/linker64 --list /aosp/wangchen/aosp/out/target/product/generic_riscv64/data/nativetest64/bionic-unit-tests/bionic-unit-tests
...... 默认方式，仅打印出依赖的 so 信息。
$ qemu-riscv64 -E LD_DEBUG=3 ./out/target/product/generic_riscv64/system/bin/bootstrap/linker64 --list /aosp/wangchen/aosp/out/target/product/generic_riscv64/data/nativetest64/bionic-unit-tests/bionic-unit-tests
...... 相比上面一条命令会打印最多的细节
```

# 4. 看一个实际的 linker 执行的例子

该实验我是基于 <https://github.com/aosp-riscv/test-riscv>，这个是我当时调试 bionic 的环境，具体使用可以参考 ["How to setup testing environment"][1]。

首先在 `LD_DEBUG=0` 的条件下执行如下命令，本质上执行的命令和上一节的类似，只不过为了能找到具体的依赖库，多一个 "-E" 的qemu 选项，具体感兴趣的可以看运行脚本。

```bash
$ cd <AOSP>/test/riscv/bionic/host
$ ./run-linker.sh 
========== Start Running ......
        libdl_android.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so (0x4003b12000)
        libdl_preempt_test_1.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so (0x4005937000)
        libdl_preempt_test_2.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so (0x400392a000)
        libdl_test_df_1_global.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so (0x4005826000)
        libtest_elftls_shared_var.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so (0x40058b6000)
        libtest_elftls_tprel.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so (0x4003a2c000)
        libc.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so (0x4004318000)
        libm.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so (0x4003982000)
        libdl.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so (0x4003a99000)
```

因为采用默认 log level，所以除了打印依赖的库信息（这个其实是程序的正常打印），log 信息并没有。

下面我们修改 `LD_DEBUG=3`（具体可以改 `<AOSP>/test/riscv/bionic/host/.run.exec` 这个文件），来看一个完整的流程。我对 log 内容做了些整理，顺便加上注释，所有注释以 ">>>>>>>>>>" 作为前缀。

```bash
========== Start Running ......
>>>>>>>>>> 打开 log 后，输出从 `linker_main` 函数开始，只有当 linker 完成了自身的 resolution 后我们才可以打印输出，以下是我们能看到的第一条 log
linker: [ Android dynamic linker (64-bit) ]
linker: [ LD_LIBRARY_PATH set to "/system/lib64/bootstrap:/system/lib64:/apex/com.android.i18n/lib64:/data/nativetest64/bionic-loader-test-libs" ]
linker: [ Linking executable "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests" ]
>>>>>>>>>> 这里对应 linker_main 函数中调用 soinfo_alloc 执行 Initialize the main exe's soinfo.
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests: allocated soinfo @ 0x4006bd3010
linker: [ Reading linker config "/linkerconfig/ld.config.txt" ]

>>>>>>>>>> 看到 “Linking” 说明开始对 exec 这个特殊的 so 调用 prelink_image
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests" ]
linker: DEBUG: si->base = 0x400241a000 si->flags = 0x40000004
>>>>>>>>>> 下面的 DEBUG 都是在 soinfo::prelink_image() 中输出的，说明 linker 在分析 exec 的 .dynamic section 每个 d 都对应一个 entry，d[0](tag) 对应的都是 DT_SONAME 这些值
linker: DEBUG: dynamic = 0x4002b5a380
linker: DEBUG: d = 0x4002b5a380, d[0](tag) = 0x1d d[1](val) = 0xe8a52
linker: DEBUG: d = 0x4002b5a390, d[0](tag) = 0x1 d[1](val) = 0xe8a74
>>>>>>>>>> ......
>>>>>>>>>> 特殊的 DT 类型，譬如 preinit/init/fini 会特别输出
linker: DEBUG: /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests constructors (DT_PREINIT_ARRAY) found at 0x4002affb28
>>>>>>>>>> ......
>>>>>>>>>> 到这基本上 prelink 阶段结束
linker: DEBUG: si->base = 0x400241a000, si->strtab = 0x40024ab590, si->symtab = 0x400241a338

linker: Trying zip file open from path "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-loader-test-libs" -> normalized "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-loader-test-libs"

>>>>>>>>>> 下面开始加载 exec 依赖的 so 并进行重定位，主要的打印都在 find_libraries 函数中输出，七个 step，但我们最关心的是 Step 1, Step 3 和 Step 6
>>>>>>>>>> Step 0: prepare. 没有什么输出
>>>>>>>>>> Step 1: expand the list of load_tasks to include all DT_NEEDED libraries
>>>>>>>>>> 一个一个依赖的 so 被尝试 open 并创建对应的 soinfo, 注意 ld-android.so 不会被重复加载
linker: [ "libdl_android.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libdl_android.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so: allocated soinfo @ 0x4006bd3268
linker: [ "libdl_preempt_test_1.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libdl_preempt_test_1.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so: allocated soinfo @ 0x4006bd34c0
linker: [ "libdl_preempt_test_2.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libdl_preempt_test_2.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so: allocated soinfo @ 0x4006bd3718
linker: [ "libdl_test_df_1_global.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libdl_test_df_1_global.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so: allocated soinfo @ 0x4006bd3970
linker: [ "libtest_elftls_shared_var.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libtest_elftls_shared_var.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so: allocated soinfo @ 0x4006bd3bc8
linker: [ "libtest_elftls_tprel.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libtest_elftls_tprel.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so: allocated soinfo @ 0x4006bd3e20
linker: [ "libc.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libc.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so: allocated soinfo @ 0x4006bd4078
linker: [ "libm.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libm.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so: allocated soinfo @ 0x4006bd42d0
linker: [ "libdl.so" find_loaded_library_by_soname failed (*candidate=n/a@0x0). Trying harder... ]
linker: [ opening libdl.so from namespace (default) ]
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so: allocating soinfo for ns=0x400011d578
linker: name /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so: allocated soinfo @ 0x4006bd4528

>>>>>>>>>> Step 2: Load libraries in random order
>>>>>>>>>> Step 3: pre-link all DT_NEEDED libraries in breadth first order.
>>>>>>>>>> 再次看到 prelink 的处理，会对每个 open 的 soinfo 执行
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so" ]
>>>>>>>>>> ......
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so" ]
>>>>>>>>>> ......
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so" ]
>>>>>>>>>> ......
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so" ]
>>>>>>>>>> ......
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so" ]
>>>>>>>>>> ......
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so" ]
>>>>>>>>>> ......
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so" ]
>>>>>>>>>> ......
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so" ]
>>>>>>>>>> ......
linker: [ Linking "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so" ]
>>>>>>>>>> ......

>>>>>>>>>> Step 4: Construct the global group.
>>>>>>>>>> Step 5: Collect roots of local_groups.
>>>>>>>>>> Step 6: Link all local groups， 这里执行真正的 relocation 操作
>>>>>>>>>> 逐个 so 进行 relocation，第一个是 exec，这里是 bionic-unit-tests，对于 linker 来说，本质上也是一个 so
>>>>>>>>>> 注意每个 so 可能存在多个类型的 relocation 的 section 需要处理，譬如 bionic-unit-tests 就存在
>>>>>>>>>> - DT_ANDROID_RELA
>>>>>>>>>> - DT_RELR/DT_ANDROID_RELR
>>>>>>>>>> - DT_JMPREL
>>>>>>>>>> 这里开始对 DT_ANDROID_RELA 的 section 进行重定位，注意这个 DT 采用 riscv64-unknown-linux-gnu-readelf 识别不了，因为这是 android 私有的
linker: DEBUG: [ android relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests ]
>>>>>>>>>> DT_RELR
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests relr ]
>>>>>>>>>> DT_RELA
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests plt rela ]
>>>>>>>>>> ......
>>>>>>>>>> 看到下面这个输出，说明至此 bionic-unit-tests 中的所有未决符号全部 relocation 完毕
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests ]

>>>>>>>>>> 下面开始对 libdl_android.so 进行处理，以此类推直到所有的 so 都处理完
	libdl_android.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so (0x400bf0a000)
linker: DEBUG: [ relocating relocating/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so ]
	libdl_preempt_test_1.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so (0x4006eac000)
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so relr ]
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so ]
	libdl_preempt_test_2.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so (0x4007026000)
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so relr ]
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so ]
	libdl_test_df_1_global.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so (0x4006f3b000)
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so relr ]
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so ]
	libtest_elftls_shared_var.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so (0x4006e29000)
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so relr ]
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so ]
	libtest_elftls_tprel.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so (0x4006f93000)
linker: DEBUG: [ android relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so ]
>>>>>>>>>> ......
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so relr ]
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so ]
	libc.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so (0x40079c4000)
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so rela ]
>>>>>>>>>> ......
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so ]
	libm.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so (0x4006d80000)
linker: DEBUG: [ android relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so ]
>>>>>>>>>> ......
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so relr ]
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so ]
	libdl.so => /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so (0x4006d20000)
linker: DEBUG: [ relocating /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so plt rela ]
>>>>>>>>>> ......
linker: DEBUG: [ finished linking /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so ]


>>>>>>>>>> linker_main() 中对 CFI 的特殊处理，也是在做 relocation，针对 __cfi_check 这个特殊的函数
linker: SEARCH __cfi_check in /system/bin/bootstrap/linker64@0x0 (gnu)
linker: NOT FOUND __cfi_check in /system/bin/bootstrap/linker64@0x0
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests@0x400241a000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests@0x400241a000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so@0x400bf0a000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl_android.so@0x400bf0a000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so@0x4006eac000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_1.so@0x4006eac000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so@0x4007026000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_preempt_test_2.so@0x4007026000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so@0x4006f3b000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libdl_test_df_1_global.so@0x4006f3b000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so@0x4006e29000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_shared_var.so@0x4006e29000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so@0x4006f93000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-loader-test-libs/libtest_elftls_tprel.so@0x4006f93000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so@0x40079c4000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libc.so@0x40079c4000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so@0x4006d80000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libm.so@0x4006d80000
linker: SEARCH __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so@0x4006d20000 (gnu)
linker: NOT FOUND __cfi_check in /aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/system/lib64/bootstrap/libdl.so@0x4006d20000

>>>>>>>>>> 以下打印标志 linker_main() 函数结束，开始返回 entry
linker: [ Ready to execute "/aosp/wangchen/aosp/out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests/bionic-unit-tests" @ 0x4002669f10 ]
```




[1]:https://github.com/aosp-riscv/test-riscv/blob/riscv64-android-12.0.0_dev/docs/howto-setup-test-env.md

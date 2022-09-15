![](./diagrams/android.png)

文章标题：**GDB 调试 Android 模拟器**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲
<!-- TOC -->

- [1. GDB 调试](#1-gdb-调试)
- [2. 改进](#2-改进)

<!-- /TOC -->

# 1. GDB 调试

最近在研究移植 Android 的 emulator，使之支持 RISC-V （rv64g）。摸索了一下使用 gdb 调试的方法，记下来备忘。

具体 emulator 环境的搭建方法请参考 [这篇](../docs/zh/howto-build-emu-riscv.md)。需要注意的是：为了支持 gdb 调试，编译 emulator 时必须加上 `--config debug` 选项。也就是说编译时要像下面这样执行：

```bash
$ cd external/qemu && android/rebuild.sh --config debug
```

接下来就可以调试了，同样以 `
sdk_phone_arm64-eng` 为例，调试的方法如下：

```bash
$ cd $AOSP/
$ . build/envsetup.sh
$ lunch sdk_phone_arm64-eng
$ cd /home/u/emu-dev/external/qemu
$ export ANDROID_BUILD_TOP=$AOSP
$ gdb -q -args objs/emulator -no-window -show-kernel -no-audio -qemu -machine virt
```

此时就会进入 gdb 的交互界面。注意 Android 的 emulator 实际上是一个封装了 QEMU 的前端。emulator 的入口函数可以参考 `android/emulator/main-emulator.cpp` 中的 `main()` 函数。这个函数主要的工作是在对我们输入的参数做初步的解析，然后封装调用了 QEMU，Android emulator 称 QEMU 为 backend，实际对应 `objs/qemu/linux-x86_64` 下的那些 `qemu-system-XXXX` 和 `qemu-system-XXXX-headless` 可执行程序，其中 headless 表示是不启动 GUI 的（`-no-window`）。具体的启动方式可以参考 `main()` 的最后面有如下代码：

```cpp
safe_execv(emulatorPath, argv);
```

进这个函数可以发现在 Linux 平台上实际就是直接调用的 `execv()`

也就是说我们启动的 emulator 这个进程会执行一会，很快就会被实际的 QEMU 进程所替换。具体调试时涉及 gdb 调试 `execv()`，但这在我使用的 gdb 8.1.1 上似乎不是问题，我们可以在启动初期对 QEMU 的代码直接设置断点。譬如 QEMU 的入口在 `android-qemu2-glue/main.cpp` 中的 `main()` 函数，我就像下面这样直接设置断点，此时 gdb 会提示找不到源码，原因很简单，我们加载的 emulator 中是不包含 `android-qemu2-glue/main.cpp` 这个文件的，但我们可以直接对 gdb 的 `Make breakpoint pending on future shared library load? (y or [n])` 直接回答 `y`，这样先强制记住这个断点，等 gdb 执行 `execv()` 加载新的程序到内存中时这个断点会自然生效。

```bash
(gdb) b android-qemu2-glue/main.cpp:main
No source file named android-qemu2-glue/main.cpp.
Make breakpoint pending on future shared library load? (y or [n]) y
Breakpoint 1 (android-qemu2-glue/main.cpp:main) pending.
(gdb)
```

下面我们可以先 `n` 或者 `s` 调试 emulator 程序部分，等要执行 `execv()` 的时候我们就直接在 gdb 中输入 `r`，让 gdb 加载 QEMU，然后就会停在我们先前设置的断点上了。

```bash
(gdb) r
Starting program: /home/u/emu-dev/external/qemu/objs/emulator -no-window -show-kernel -no-audio -qemu -machine virt
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
INFO    | Android emulator version 31.2.1.0 (build_id ) (CL:N/A)
process 553471 is executing new program: /home/u/emu-dev/external/qemu/objs/qemu/linux-x86_64/qemu-system-aarch64-headless
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/x86_64-linux-gnu/libthread_db.so.1".
[New Thread 0x7ffff5aff700 (LWP 553476)]

Thread 1 "qemu-system-aar" hit Breakpoint 1, main (argc=7, argv=0x7fffffffd268) at ../android-qemu2-glue/main.cpp:1193
1193        if (argc < 1) {
(gdb)
```

# 2. 改进

在调试的时候我发现每次重新启动终端后都需要把到 AOSP 源码树目录下去执行 lunch，然后再切换回 emulator 目录下运行调试。这个有点麻烦，有没有办法省略掉这个操作能？我研究了一下 emulator 中 QEMU 的源码，发现之所以需要做这个操作，主要目的是为了产生 `ANDROID_PRODUCT_OUT` 这个环境变量的值。具体参考 `createAVD()` 这个函数，定义在 `android/android-emu/android/main-common.c` 中。具体的调用路径如下：

```
main -> emulator_parseCommonCommandLineOptions -> createAVD
```

在 `createAVD()` 中会尝试获取 `ANDROID_PRODUCT_OUT` 这个环境变量的值，而我们运行 lunch 就会产生并导出这个环境变量。

所以为了避免 lunch，我们也可以自己定义 `ANDROID_PRODUCT_OUT`，这个环境变量用于指定 `target-specific out directory where disk images will be looked for`。具体到我们这里的操作，对应的就是 `$AOSP/out/target/product/emulator_arm64`。

为方便每次启动 gdb 调试都要输入太多的命令，我们把所有操作都写到 gdbinit 文件中去

假设 `$AOSP` 是 `/home/u/aosp12`

```bash
$ cat ./gdbinit
set env ANDROID_BUILD_TOP=/home/u/aosp12
set env ANDROID_PRODUCT_OUT=/home/u/aosp12/out/target/product/emulator_arm64
set args -no-window -show-kernel -no-audio -qemu -machine virt
file /home/u/emu-dev/external/qemu/objs/emulator
set breakpoint pending on
b android-qemu2-glue/main.cpp:main
```

其中 `set breakpoint pending on` 的作用就是替代我们对下面暂时不存在的断点 `b android-qemu2-glue/main.cpp:main` 回答 yes。

然后我们就可以这样只需要输入下面两条命令就可以直接启动 gdb 了

```bash
$ cd /home/u/emu-dev/external/qemu
$ gdb -q -x <path-to-gdbinit>
```




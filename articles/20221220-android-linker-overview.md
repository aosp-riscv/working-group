![](./diagrams/android-riscv.png)

文章标题：**Android Linker 总览**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

Android 的 bionic 仓库中实现了 Android 系统的 Dynamic linker（bionic 中也简称 linker，本文中如果提及 linker 也指 dynamic linker）。

linker 的具体代码在 `<AOSP>/bionic/linker` 下。下文以 `<AOSP>` 指代 aosp 采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

# linker 概览

整体上来说，Android Linker 实现了基于 Linux 以及 ELF 格式的 [Dynamic linker][2] 和 [Dynamic loading][3] 的功能。

[Dynamic linker][2] 的基本概念是指，当我们调用操作系统的系统调用 execve 时，操作系统根据 program 的 Program Headers 中的 `PT_INTERP` 指定的 dynamic linker 路径将其先加载到内存中，然后将执行权交给 dynamic linker，dynamic linker 负责将应用程序以及其依赖的 so 加载（loading）到内存中并完成 symbol relocation 的操作。

[Dynamic loading][3] 的基本概念是，能让程序在运行时（而不是编译时）加载 so 库到内存中（`dlopen()`）并完成 symbol relocation 的操作，我们可以根据符号获取库中函数和变量的地址（`dlsym()`），并执行这些函数或访问这些变量，且能在不需要时将库从内存中卸载（`dlclose()`）。在 GNU/Linux 上 [Dynamic loading][3] 的功能是由 libdl.so 提供的，而在 Android/Linux 上，libdl.so 只是一个 wrapper，实际功能实现在 linker 中，复用了 linker 的一部分代码。

综上所述，Android linker 包含了 [Dynamic linker][2] 和 [Dynamic loading][3] 两部分的实现，实现的主要功能可以总结为以下两点：

- load：加载可执行程序依赖的库到内存中并完成 memory map；
- link：这里的 link 完成的工作即对被引用的符号完成重定向（relocation）。重定向可以简单理解成，在 GOT 表中预留一个指针来间接引用动态链接库中的符号（包括全局变量和函数），linker 负责在运行时 load 完成后确定这些符号的实际加载地址，然后修改这个指针，使其指向正确的加载地址。

# load

有关加载库的搜索。ELF 中 `.dynamic` section 中的 `DT_NEED` 表项记录的只是文件名，没有包含完整路径，那么在哪里找到这些文件呢？另外，dlopen 函数参数指定要加载的库文件可以是绝对路径，也可以是不带路径的文件名，同样存在如何查找的问题。

Linker 解决定位动态库的基本思路是：Linker 会按照一定的顺序查找一些指定的目录下是否存在指定的库文件。同时 Android 的 linker 在 Android N 版本上还引入了一个命名空间的概念，使库文件的查找变得稍微复杂一下，但是基本的查找原则是一致的。这里先介绍引入命名空间之前的查找规则，有关命名空间的概念，限于篇幅会另外起一篇文字总结。

Linker 按照顺序在指定的一些目录中查找依赖的库文件，这个顺序受运行时的环境变量、编译时的参数，以及 linker 内部实现影响。查找顺序的规则如下。

- 如果环境变量 `LD_LIBRARY_PATH=/path/to/dir1/:/path/to/dir2/` 被设置，则首先根据该环境变量在指定的目录中顺序查找；
- 如果库文件编译时使用了 `-rpath=/path/to/dir1:/path/to/dir2`, 则在 rpath 参数指定的目录中查找。rpath 指定的路径保存在 ELF 文件的动态段中的 RUNTPATH 表项中：
- 最后在 linker 指定的默认路径中查找。不同的操作系统或者不同的 linker 实现，有不同的配置。Android 10 系统上如果没有配置命名空间规则(实际都会配置，这里只是举个简单例子），则默认的查找路径如下：/system/lib64:/odm/lib64:/vendor/lib64

被依赖的库文件，也可能依赖其他的库文件，Linker 在加载时按照 BFS 顺序，加载这些库文件到进程的内存地址空间。

# link

Linker 将所有依赖涉及的库文件全部加载到进程的内存地址空间之后，就开始对外部引用的符号进行重定位。这个过程就比较直观了，大致过程如下：从可执行程序或者 dlopen 要加载的库开始，按照 BFS 顺序遍历每个加载的库文件；对于每个库文件，遍历所有的重定向表，对于每个表项，在依赖的库中查找符号的地址，将符号地址写入表项指定的地址，完成符号解析工作。

# load 和 link 的具体实现

load 和 link 的具体实现在代码中对应的就是 `find_libraries()` 这个函数，该函数定义在 `<AOSP>/bionic/linker/linker.cpp` 中。这个函数是 Android 的 linker 和 libdl 中复用的核心函数，实现了 linker 加载库函数，重定位符号的主要过程，是 linker 中极为重要的一个函数，也是理解 linker 运行原理的关键之一。

`find_libraries()` 在两处被调用：

一处是 `linker_main() -> find_libraries()`：这里是正常的应用程序执行过程中 linker 初始化阶段完成自身 relocation 后开始加载 `DT_NEEDED` 的 so。

还有一处是走 dlopen 的逻辑，最终的执行函数入口在 `do_dlopen()`, 在 `do_dlopen()` 中发生 `do_dlopen() -> find_library() -> find_libraries()`。

而 `do_dlopen()` 会有两条路径会被调用到：
- `android_dlopen_ext() -> __loader_android_dlopen_ext() -> dlopen_ext() -> do_dlopen()`
- `dlopen() -------------> __loader_dlopen() -------------> dlopen_ext() -> do_dlopen()`

`android_dlopen_ext()` 是 Android 上自己对 POSIX API `dlopen()` 的扩充。

注意从代码组织上，`do_dlopen()` 及其以后的函数都是定义在 `<AOSP>/bionic/linker/linker.cpp`， `__loader_` 开头的函数则定义在 `<AOSP>/bionic/linker/dlfcn.cpp`，`android_dlopen_ext()` 和 `dlopen()` 则定义在 `<AOSP>/bionic/libdl` 下，所以在 Android 里，libdl 的实现主体实际上是在 linker 中，之所以还要提供一个 libdl，完全是为了兼容 POSIX 标准。

限于篇幅，`find_libraries()` 这个函数的总结我会另起一篇。

有关 Android linker 的入口函数的介绍，参考笔记 [《Android Dynamic Linker 的入口》][1]。在这篇文字中我们知道 `_start` 函数实际上没敢啥，就是调用了 `__linker_init` 这个函数，这个函数定义在 `<AOSP>/bionic/linker/linker_main.cpp`。

linker 作为一个动态链接应用程序执行过程中启动阶段的一个过渡过程，最终还是会将执行权交给用户编写的 main，所以 linker 的所有工作其实就在 `__linker_init` 这个函数中一次性完成了。当 `__linker_init` 执行结束（无论成功还是失败）后，linker 的使命也就完成了。

本文先从整体上总结一下 linker （即 `__linker_init` 函数）做了些啥。

整体上 `__linker_init` 按照是否完成自身的 relocation 分为两个阶段：

- 第一阶段的 `fixing the linker's own relocations`，即解决 linker 自身的重定位问题。在解决所有的符号重定位问题之前，`__linker_init()` 函数不可以引用任何 linker 模块外部的 extern 的变量和 extern 的函数（特别地，譬如调用 heap 内存分配等操作），在自身重定位没有完成时，GOT 中对应 extern 变量和 extern 函数的地址都是非法的，访问这些外部符号会 `generate a segfault`。`__linker_init` 函数在调用 `__linker_init_post_relocation()` 函数之前的所有代码都可以归为第一阶段。
  
- 第二阶段也就是在解决了 `linker's own relocations` 之后会调用 `__linker_init_post_relocation()`，从函数名字上可以看出来这个函数就是在解决了 relocation 之后（post）再进一步执行一些初始化工作。这个函数内部还以调用 `linker_main` 函数为界分为两个子阶段。


# 
# 几个重要的 static 的 对象




[1]:./20221220-andorid-linker-entry.md
[2]:https://en.wikipedia.org/wiki/Dynamic_linker
[3]:https://en.wikipedia.org/wiki/Dynamic_loading


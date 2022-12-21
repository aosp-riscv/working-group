![](./diagrams/android-riscv.png)

文章标题：**Android Dynamic Linker 流程总览**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

Android 的 bionic 仓库中实现了 Android 系统的 Dynamic linker（bionic 中也简称 linker，本文中如果提及 linker 也指 dynamic linker）。

linker 的具体代码在 `<AOSP>/bionic/linker` 下。下文以 `<AOSP>` 指代 aosp 采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

# linker 执行流程总览

有关 Android linker 的入口函数的介绍，参考笔记 [《Android Dynamic Linker 的入口》][1]。在这篇文字中我们知道 `_start` 函数实际上没敢啥，就是调用了 `__linker_init` 这个函数，这个函数定义在 `<AOSP>/bionic/linker/linker_main.cpp`。

linker 作为一个动态链接应用程序执行过程中启动阶段的一个过渡过程，最终还是会将执行权交给用户编写的 main，所以 linker 的所有工作其实就在 `__linker_init` 这个函数中一次性完成了。当 `__linker_init` 执行结束（无论成功还是失败）后，linker 的使命也就完成了。

本文先从整体上总结一下 linker （即 `__linker_init` 函数）做了些啥。

整体上 `__linker_init` 按照是否完成自身的 relocation 分为两个阶段：

- 第一阶段的 `fixing the linker's own relocations`，即解决 linker 自身的重定位问题。在解决所有的符号重定位问题之前，`__linker_init()` 函数不可以引用任何 linker 模块外部的 extern 的变量和 extern 的函数（特别地，譬如调用 heap 内存分配等操作），在自身重定位没有完成时，GOT 中对应 extern 变量和 extern 函数的地址都是非法的，访问这些外部符号会 `generate a segfault`。`__linker_init` 函数在调用 `__linker_init_post_relocation()` 函数之前的所有代码都可以归为第一阶段。
  
- 第二阶段也就是在解决了 `linker's own relocations` 之后会调用 `__linker_init_post_relocation()`，从函数名字上可以看出来这个函数就是在解决了 relocation 之后（post）再进一步执行一些初始化工作。这个函数内部还以调用 `linker_main` 函数为界分为两个子阶段。


# 
# 几个重要的 static 的 对象




[1]:./20221220-andorid-linker-entry.md


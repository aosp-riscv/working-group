![](./diagrams/android-riscv.png)

文章标题：**Android Dynamic Linker 初始化流程总览**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

Android 的 bionic 仓库中实现了 Android 系统的 Dynamic linker（bionic 中也简称 linker，本文中如果提及 linker 也指 dynamic linker）。

linker 的具体代码在 `<AOSP>/bionic/linker` 下。下文以 `<AOSP>` 指代 aosp 采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

在 Unix-like 系统上，譬如 Linux，动态链接的实现方式就是在程序加载时首先加载所谓的 dynamic linker，然后程序处理跳转到 dynamic linker 的入口函数地址处由其执行后继的 load 和 linker 共享库的操作。

有关 Android linker 的入口函数的介绍，参考笔记 [《Android Dynamic Linker 的入口》][1]。在这篇文字中我们知道 `_start` 函数实际上没干啥，就是调用了 `__linker_init` 这个函数，这个函数定义在 `<AOSP>/bionic/linker/linker_main.cpp`。

linker 作为一个动态链接应用程序执行过程中启动阶段的一个过渡过程，最终还是会将执行权交给用户编写的 main，所以 linker 的所有工作其实就在 `__linker_init` 这个函数中一次性完成了。当 Android Dynamic Linker 初始化执行结束（无论成功还是失败）后，linker 的使命也就完成了。

本文先从整体上总结一下 linker （即 `__linker_init` 函数）做了些啥。

整体上 `__linker_init` 按照是否完成自身的 relocation 分为两个阶段，代码上的注释写得也比较清楚了。

```cpp
/*
 * This is the entry point for the linker, called from begin.S. This
 * method is responsible for fixing the linker's own relocations, and
 * then calling __linker_init_post_relocation().
 *
 * Because this method is called before the linker has fixed it's own
 * relocations, any attempt to reference an extern variable, extern
 * function, or other GOT reference will generate a segfault.
 */
extern "C" ElfW(Addr) __linker_init(void* raw_args) {

  // 第一阶段：解决 linker 自身的 relocation 问题，
  代码省略 ......

  // 第二阶段
  return __linker_init_post_relocation(args, tmp_linker_so);
}
```

- 第一阶段按照代码的注释是 `fixing the linker's own relocations`，即解决 linker 自身的重定位问题。我们知道，dynamic linker 本身也是一个 so，但和其他的普通的 so 相比在 load 阶段存在很大的不同。

  其他普通的 so 的 load 问题由 dynamic linker 解决，其中主要解决以下两个问题：
  - 如果该 so 依赖于另外的 so，那么 dynamic linker 在 load 该 so 的时候还会将其依赖的 so 也 load 进来。
  - 其次解决主程序和 so 的 relocation 问题。

  而 dynamic linker 作为一个 so，只能靠自己解决自身的以上问题。

  - 对于依赖问题，为避免复杂性，dynamic linker 本身要避免依赖于其他任何的 so。我们可以运行 `llvm-readelf -d out/target/product/generic_riscv64/system/bin/bootstrap/linker64 | grep NEEDED` 进行检查，结果可以发现 android 的 linker 果然不依赖任何其他的 so。

  - 在 relocation 问题上，在排除了不会依赖其他 so（模块）的基础上，同样为避免复杂性，dynamic linker 要尽量避免引入访问 so 内的非静态（static）全局函数或者变量就好了。

    对于 static 函数和变量，比较好处理，因为针对 so 内部的 static 符号，编译器会生成相对 PC 的 offset 访问指令，不涉及运行时的 relocation。我们可以发现 `<AOSP>/bionic/linker` 下代码中大量使用了 static 方式。

    对于必须使用的非静态函数，android 采用的方法是：在编译源文件时使用了 `-ffunction-sections -fdata-sections` 这样的编译选项，在编译生成的 `.o` 目标文件中，会将每个函数或数据段，放在各种单独独立的 section 中；生成 `.a` 后在链接生成 linker 时，采用了 `-Bstatic -Wl,--gc-sections` 这样形式的选项，指示链接器仅将需要的函数级别的指令摘出来和主程序的目标文件进行静态链接（普通的 `-static` 只能做到 .o 级别摘取），这样不仅避免了运行期的 relocation，也大大缩小了 linker 的体积。

    针对非静态的全局变量访问，仍然会生成 GOT。`__linker_init` 需要解决的符号重定位问题主要就是指的这里，在没有 `fixing the linker's own relocations` 之前，GOT 中对应的变量地址都是非法的，访问这些全局符号会 `generate a segfault`。此外需要注意的是 Android 为了缩小 so 的体积，采用了一个自己定义的专为 relocation 优化的 section type：`SHT_RELR`。值是 19， 定义在 `<AOSP>/bionic/libc/include/elf.h`，不为 GNU 的 binutils 所识别。使用时需要注意一下，具体了解，可以看 [Proposal for a new section type SHT_RELR][2]。

    `__linker_init` 函数在调用 `__linker_init_post_relocation()` 函数之前的所有代码都可以归为第一阶段。

- 第二阶段也就是在解决了 `linker's own relocations` 之后会调用 `__linker_init_post_relocation()`，从函数名字上可以看出来这个函数就是在解决了 relocation 之后（post）再进一步执行一些初始化工作。这个函数内部还以调用 `linker_main` 函数为界分为两个子阶段。

针对第一阶段和第二阶段详细的代码分析，我打算另外分别写一篇总结，就不在本文里赘述了。

[1]:./20221220-andorid-linker-entry.md
[2]:https://groups.google.com/d/msg/generic-abi/bX460iggiKg/Pi9aSwwABgAJ

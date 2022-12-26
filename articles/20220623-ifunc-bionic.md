![](./diagrams/linker-loader.png)

文章标题：**BIONIC 中对 IFUNC 的支持**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 参考](#1-参考)
- [2. 引言](#2-引言)
- [3. 动态链接方式](#3-动态链接方式)
- [4. 静态链接方式](#4-静态链接方式)

<!-- /TOC -->

修改历史：

- 2022/06/23: 初版
- 2022/12/26: 参考代码基线更新，补充了动态链接下的处理

# 1. 参考

本文主要参考了如下内容：

- 【参考 1】[GNU IFUNC 介绍（RISC-V 版）][1]

# 2. 引言

本文结合我在移植 AOSP 项目的 bionic 到 RISC-V 过程中的工作以及对 IFUNC 的学习，简单介绍一下 bionic 中是如何支持 IFUNC 这个特性的。

参考我对 IFUNC 特性的总结，见 [【参考 1】][1]，我们知道为了实现 IFUNC 这个特性，整个工具链，包括运行库都需要做相应的支持，其中运行库就是我们常说的 C 库了，在 AOSP 这个系统中，它使用了自己的 C 库，就是 bionic。下面我结合 bionic 的代码，简单总结一下在动态链接和静态链接两种工作模式下 C 库是如何实现对 IFUNC 的。

本文使用的代码基于 AOSP 官方主线仓库 <https://android.googlesource.com/platform/bionic/>。描述代码文件路径时，定义 `<AOSP>` 指代采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

# 3. 动态链接方式

和 Glibc 有所不同的是，Bionic 并不支持延迟绑定，bionic 的 dynamic linker 会在程序初始化阶段，即加载共享库的过程中对所有的未决符号完成重定位的过程，对于 ifunc 也是一样的处理。完整的加载和重定位过程就不在本文中描述了。我们直接定位到处理重定位的核心函数 `process_relocation_impl()`，这个文件的位置在 `<AOSP>/bionic/linker/linker_relocate.cpp`。

这个函数根据 relocation 的 type 执行不同的处理，就是一堆 switch-case 结构。摘录处理 `R_RISCV_IRELATIVE` 的逻辑如下：

```cpp
  switch (r_type) {
    case R_GENERIC_IRELATIVE:
      // In the linker, ifuncs are called as soon as possible so that string functions work. We must
      // not call them again. (e.g. On arm32, resolving an ifunc changes the meaning of the addend
      // from a resolver function to the implementation.)
      if (!relocator.si->is_linker()) {
        count_relocation_if<IsGeneral>(kRelocRelative);
        const ElfW(Addr) ifunc_addr = relocator.si->load_bias + get_addend_rel();
        trace_reloc("RELO IRELATIVE %16p <- %16p",
                    rel_target, reinterpret_cast<void*>(ifunc_addr));
        if (handle_text_relocs && !protect_segments()) return false;
        const ElfW(Addr) result = call_ifunc_resolver(ifunc_addr);
        if (handle_text_relocs && !unprotect_segments()) return false;
        *static_cast<ElfW(Addr)*>(rel_target) = result;
      }
      break;
```

其中代码中的 `R_GENERIC_IRELATIVE` 是一个宏，针对不同的 ARCH 它会被重定义，具体重定义的地方在 `<AOSP>/bionic/linker/linker_relocs.h`, 针对 riscv64，代码示例如下：

```cpp
#elif defined (__riscv)
......
#define R_GENERIC_IRELATIVE     R_RISCV_IRELATIVE
......
```

我们看到上面对 `R_RISCV_IRELATIVE` 进行重定位处理的过程正如我们在 [【参考 1】][1] 中介绍的那样，在计算得到重定向的符号地址 `ifunc_addr` 后并没有直接给 GOT 项赋值（感兴趣的同学可以对比 `process_relocation_impl()` 中针对 `R_GENERIC_JUMP_SLOT` 的处理），而是继续根据这个地址调用 resolver 函数，具体实现在 `call_ifunc_resolver()` 中，以这个函数的返回值作为 `result` 填写 GOT 项。

以上是普通 so 的 ifunc resolve 处理，由 dynamic linker 完成，而对于 dynamic linker 自身，由于 dynamic linker 自身本质上也是一个 so，那么它自己的 ifunc 重定位处理又是由谁来完成呢？

没有其他人的帮助，只能是自己解决。在 dynamic linker 初始化的过程中，也就是在 `__linker_init()` 函数中，我们可以看到有相关处理调用。相关文件在 `<AOSP>/bionic/linker/linker_main.cpp`：

```cpp
extern "C" ElfW(Addr) __linker_init(void* raw_args) {
  ......
  // string.h functions must not be used prior to calling the linker's ifunc resolvers.
  call_ifunc_resolvers();
  ......
}
```

`call_ifunc_resolvers()` 是由 linker 自身提供的一个 static 函数，解决了 linker 自己的 ifunc 重定位问题。

# 4. 静态链接方式

如果程序是静态链接的，自然没有动态链接器配合其进行运行期间的重定位过程，更不可能在静态链接阶段来做重定位，因为引入 IFUNC 的目的就是在运行期间进行选择（resolve）。因此，如果一个程序是静态链接的，则 resolve 的工作由 libc 来代替动态链接器完成此操作。具体设计思想如下：

- 从链接器（譬如 ld）的角度来说，在执行静态链接过程中，依然负责为 IFUNC 符号生成重定位信息，但除此之外，链接器会新增了两个符号 `__rela_iplt_start` 和 `__rela_iplt_stop` 来标识出 `R_RISCV_IRELATIVE` 对应的重定位表项（可以认为是一个 `Elf64_Rela` 类型的结构体数组）加载到内存后的起始和结束地址。
- 一个静态链接的程序在执行 `main()` 函数之前会有一段初始化过程，在此期间，通过 `__rela_iplt_start` 和 `__rela_iplt_stop` 这两个符号定位到内存中和 IFUNC 有关的重定位表项，然后执行类似在动态链接方式下由动态链接器执行的重定位过程。

依据以上思路，我们来看一下对应的实例。以 bionic 单元测试的 unit test 中的 `bionic-unit-tests-static`（静态链接版本）为例。ifunc 测试中会定义两个 ifunc 函数 `ifunc()` 和 `hwcap()`， 参考 `<AOSP>/bionic/tests/ifunc_test.cpp`

```cpp
......
int ifunc() __attribute__((ifunc("resolver")));
......
int hwcap() __attribute__((ifunc("hwcap_resolver")));
......
```

先找一下可执行程序中的 `__rela_iplt_start` 和 `__rela_iplt_stop` 这两个符号。

```bash
$ llvm-readelf -s out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests-static/bionic-unit-tests-static | grep rela_iplt_end
1850419: 00000000000102d8     0 NOTYPE LOCAL  HIDDEN      3 __rela_iplt_end
$ llvm-readelf -s out/target/product/generic_riscv64/symbols/data/nativetest64/bionic-unit-tests-static/bionic-unit-tests-static | grep rela_iplt_start
1850420: 00000000000102a8     0 NOTYPE LOCAL  HIDDEN      3 __rela_iplt_start
```

这里链接器告诉我们这个程序中存在 IFUNC relocation entrys，加载到内存后起始地址是 0x102d8，结束地址是 0x102a8，间隔 48 个字节，正好占 2 个 `Elf64_Rela` 大小，对应上面代码中的两个 ifunc 函数。

验证一下，先看下程序的 relocation table，发现的确存在两项：
```bash
llvm-readelf -r out/target/product/generic_riscv64/data/nativetest64/bionic-unit-tests-static/bionic-unit-tests-static 

Relocation section '.rela.plt' at offset 0x2a8 contains 2 entries:
    Offset             Info             Type               Symbol's Value  Symbol's Name + Addend
00000000005d05a8  000000000000003a R_RISCV_IRELATIVE                 171198
00000000005d05b0  000000000000003a R_RISCV_IRELATIVE                 1712ac
```

再验证一下这两项加载到内存中的地址是否和符号表中的 Value 对应。查看 program headers table：

```bash
llvm-readelf -lW out/target/product/generic_riscv64/data/nativetest64/bionic-unit-tests-static/bionic-unit-tests-static 

Elf file type is EXEC (Executable file)
Entry point 0xe4250
There are 10 program headers, starting at offset 64

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  PHDR           0x000040 0x0000000000010040 0x0000000000010040 0x000230 0x000230 R   0x8
  LOAD           0x000000 0x0000000000010000 0x0000000000010000 0x0d3250 0x0d3250 R   0x1000
  LOAD           0x0d3250 0x00000000000e4250 0x00000000000e4250 0x483380 0x483380 R E 0x1000
  LOAD           0x5565d0 0x00000000005685d0 0x00000000005685d0 0x067fe8 0x067fe8 RW  0x1000
  LOAD           0x5be5b8 0x00000000005d15b8 0x00000000005d15b8 0x18b2c8 0x6ee8a8 RW  0x1000
  TLS            0x5565d0 0x00000000005685d0 0x00000000005685d0 0x000020 0x000024 R   0x8
  GNU_RELRO      0x5565d0 0x00000000005685d0 0x00000000005685d0 0x067fe8 0x068a30 R   0x1
  GNU_EH_FRAME   0x062498 0x0000000000072498 0x0000000000072498 0x017b94 0x017b94 R   0x4
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x0
  NOTE           0x000270 0x0000000000010270 0x0000000000010270 0x000038 0x000038 R   0x4

 Section to Segment mapping:
  Segment Sections...
   00     
   01     .note.android.ident .note.gnu.build-id .rela.plt .rodata .gcc_except_table .eh_frame_hdr .eh_frame 
   02     .text .iplt 
   03     .tdata .preinit_array .init_array .fini_array .data.rel.ro .got .got.plt 
   04     .sdata .data .bss .sbss 
   05     .tdata .tbss 
   06     .tdata .preinit_array .init_array .fini_array .data.rel.ro .got .got.plt 
   07     .eh_frame_hdr 
   08     
   09     .note.android.ident .note.gnu.build-id 
   None   .riscv.attributes .shstrtab .gnu_debugdata
```

可见 `.rela.plt` 这个 section 处于 01 这个 segment 中，而这个 segment 的加载地址在 0x10000，在加上前面 relocation table 中的 offset 值 0x2a8，和我们符号表中的起始地址 0x102a8 也是匹配的。

最后我们来看一下 bionic 代码中执行 resolve 的地方，静态链接的程序的入口执行顺序大致如下：

```cpp
_start              // <AOSP>/bionic/libc/arch-common/bionic/crtbegin.c
-> _start_main      // <AOSP>/bionic/libc/arch-common/bionic/crtbegin.c
-> __libc_init      // <AOSP>/bionic/libc/bionic/libc_init_static.cpp
-> __real_libc_init // <AOSP>/bionic/libc/bionic/libc_init_static.cpp, 
                    // 这个函数执行 main 之前的初始化工作，最后会跳
                    // 转到 main 函数去执行，直到程序结束才会返回。
```

而在 `__real_libc_init()` 函数中，我们看到这么一处函数调用:

```cpp
__noreturn static void __real_libc_init(void *raw_args,
                                        void (*onexit)(void) __unused,
                                        int (*slingshot)(int, char**, char**),
                                        structors_array_t const * const structors,
                                        bionic_tcb* temp_tcb) {
  ......

  call_ifunc_resolvers();
  ......
```

跟进去，我们看到正是在这个函数中实现了利用 `__rela_iplt_start` 和 `__rela_iplt_end` 这两个符号遍历 relocation table 以及调用 resolver 的过程。

```cpp
#if defined(__arm__) || defined(__i386__)  // Legacy architectures used REL...
  省略 ......
#else  // ...but modern architectures use RELA instead.
extern __LIBC_HIDDEN__ __attribute__((weak)) ElfW(Rela) __rela_iplt_start[], __rela_iplt_end[];

static void call_ifunc_resolvers() {
  if (__rela_iplt_start == nullptr || __rela_iplt_end == nullptr) {
    // These symbols were not emitted by gold. Gold has code to do so, but for
    // whatever reason it is not being run. In these cases ifuncs cannot be
    // resolved, so we do not support using ifuncs in static executables linked
    // with gold.
    //
    // Since they are weak, they will be non-null when linked with bfd/lld and
    // null when linked with gold.
    return;
  }

  for (ElfW(Rela)* r = __rela_iplt_start; r != __rela_iplt_end; ++r) {
    ElfW(Addr)* offset = reinterpret_cast<ElfW(Addr)*>(r->r_offset);
    ElfW(Addr) resolver = r->r_addend;
    *offset = __bionic_call_ifunc_resolver(resolver);
  }
}
#endif
```

[1]: ./20220621-ifunc.md

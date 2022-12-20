![](./diagrams/android-riscv.png)

文章标题：**Android Dynamic Linker 的入口**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

Android 的 bionic 仓库中实现了 Android 系统的 Dynamic linker（bionic 中也简称 linker，本文中如果提及 linker 也指 dynamic linker）。

linker 的具体代码在 `<AOSP>/bionic/linker` 下。下文以 `<AOSP>` 指代 aosp 采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

和 GNU/Linux 下的 dynamic linker 类似，Android 的 dynamic linker 由 kernel 在加载可执行程序时调用。Android 的可执行程序为 ELF 格式，ELF 可执行程序有一个 INTERP 类型的 program header 项，指定了 linker 程序的路径。以 bionic 的单元测试程序 bionic-unit-tests 为例看一下，这个程序是测试动态链接版本的测试用例的。

```bash
$ llvm-readelf -l out/target/product/generic_riscv64/data/nativetest64/bionic-unit-tests/bionic-unit-tests 

Elf file type is DYN (Shared object file)
Entry point 0x24ff10
There are 12 program headers, starting at offset 64

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  PHDR           0x000040 0x0000000000000040 0x0000000000000040 0x0002a0 0x0002a0 R   0x8
  INTERP         0x0002e0 0x00000000000002e0 0x00000000000002e0 0x00001f 0x00001f R   0x1
      [Requesting program interpreter: /system/bin/bootstrap/linker64]
  LOAD           0x000000 0x0000000000000000 0x0000000000000000 0x24ef10 0x24ef10 R   0x1000
  省略 ......
```

我们看到 program interpreter 的路径为 `/system/bin/bootstrap/linker64`。这就是 android 的 dynamic linker 所在的路径。

当在命令行中运行一个 ELF 可执行程序的时候，内核会分析可执行程序的 program header table 中的 INTERP 项并根据这个字段中的值（是 linker 的路径）找到 linker（linker 本身也是一个 ELF 格式的动态库文件）并将其加载到内存，然后跳转到 linker 程序的入口函数执行，linker 先负责完成动态连接过程：这包括了加载库，并完成动态库中符号的重定向。最后 linker 程序跳转到可执行程序的入口 main 处开始执行。看上去就像程序直接运行一样。

查看 linker64 的 Elf header 的 "Entry point address" 信息可以获取 linker 的入口地址：

```bash
$ llvm-readelf -h ./out/target/product/generic_riscv64/system/bin/bootstrap/linker64
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              DYN (Shared object file)
  Machine:                           RISC-V
  Version:                           0x1
  Entry point address:               0x5B6F0
  Start of program headers:          64 (bytes into file)
  Start of section headers:          6797080 (bytes into file)
  Flags:                             0x5, RVC, double-float ABI
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         10
  Size of section headers:           64 (bytes)
  Number of section headers:         25
  Section header string table index: 22
```

可见 linker 的 entry point 地址为 0x5B6F0，看一下该地址对应的指令：

```
$ llvm-objdump -d ./out/target/product/generic_riscv64/system/bin/bootstrap/linker64 | grep 5b6f0
000000000005b6f0 <__dl__start>:
   5b6f0: 0a 85         mv      a0, sp
```

代码中搜索 `"__dl__start"` 这个符号却找不到。阅读 `<AOSP>/bionic/linker/Android.bp` 发现原来构建过程中认为给符号加上了前缀 `"__dl_"`。

```json
// A template for the linker binary. May be inherited by native bridge implementations.
cc_defaults {
    name: "linker_bin_template",

    省略 ......

    // Insert an extra objcopy step to add prefix to symbols. This is needed to prevent gdb
    // looking up symbols in the linker by mistake.
    prefix_symbols: "__dl_",
```

所以真实的 linker 入口就是 `_start`，这个符号定义是 ARCH 相关的，针对 riscv 定义在 `<AOSP>/bionic/linker/arch/riscv64/begin.S` 文件中。

```cpp
ENTRY(_start)
  // Force unwinds to end in this function.
  .cfi_undefined ra

  mv a0, sp
  jal __linker_init

  // __linker_init returns the address of the entry point in the main image.
  jr a0
END(_start)
```

在 `_start` 函数中，栈顶指针寄存器 sp 被赋值给 a0 寄存器作为参数调用 `__linker_init()`。 `__linker_init()` 执行 linker 的初始化工作做完后, 通过 `a0` 返回了可执行文件入口函数地址，接下来的 `jr a0` 指令就会跳转到可执行文件定义的入口，即我们编写的 main 函数处执行。这部分代码和 ARCH 的 ABI call convertion 有关，所以采用汇编方式编写。





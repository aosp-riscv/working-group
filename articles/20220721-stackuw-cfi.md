![](./diagrams/linker-loader.png)

文章标题：**Stack Unwinding 之基于 Call Frame Information**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 参考](#1-参考)
- [2. 基本概念](#2-基本概念)
    - [2.1. DWARF 定义的 section `.debug_frame`](#21-dwarf-定义的-section-debug_frame)
    - [2.2. LSB 定义的 section `.eh_frame`](#22-lsb-定义的-section-eh_frame)
    - [2.3. `.eh_frame` vs `.debug_frame`](#23-eh_frame-vs-debug_frame)
    - [2.4. 编译选项对 section `.debug_frame` 和 `.eh_frame` 的影响](#24-编译选项对-section-debug_frame-和-eh_frame-的影响)
- [3. 基于 CFI 的栈回溯工作原理分析](#3-基于-cfi-的栈回溯工作原理分析)
- [4. section `.eh_frame` 中的数据结构](#4-section-eh_frame-中的数据结构)
- [5. CFI directives](#5-cfi-directives)
- [6. 通过 glibc 的 backtrace 函数实现栈回溯的例子](#6-通过-glibc-的-backtrace-函数实现栈回溯的例子)

<!-- /TOC -->

# 1. 参考

本文主要参考了如下内容：

- 【参考 1】[Call Stack (RISC-V)][1]
- 【参考 2】[Stack Unwinding - Overview][2]
- 【参考 3】[Stack unwinding][3]
- 【参考 4】[AARCH64平台的栈回溯][4]
- 【参考 5】[Unwind 栈回溯详解][5]


本文主要总结基于 Call Frame Information 实现栈回溯（Stack Unwinding）的工作原理，至于什么是 “Stack” 以及什么是 “Stack Unwinding”，请阅读我的另外两篇总结 [《Call Stack (RISC-V)》][1] 和 [《Stack Unwinding - Overview》][2]。

# 2. 基本概念

在 [《Stack Unwinding 之基于 Frame Pointer》][6] 这篇笔记中我们了解到基于 frame pointer 实现 Stack Unwinding 虽然十分方便，但有自己的不足，回顾一下：

- 需要一个专门寄存器来保存 frame poniter，在 RISC-V 上是 x8 (其 ABI name 是 s0 或者 fp) 。
- 函数的 prologue 和 epilogue 中会有额外的 push fp 寄存器或者 pop fp 寄存器信息的动作，实际执行时增大了指令开销。
- 在栈回溯过程中，能够恢复的寄存器有限，除了 sp/fp/ra 之外不知道怎么恢复其他的寄存器，这可能导致无法支持譬如 gdb 运行过程中执行 info reg 这类功能。
- 依赖于 ARCH 的行为，譬如某种 ARCH 因为寄存器不够用无法专门预留一个 fp 寄存器。
- 没有源语言信息。

那么有没有什么方法可以突破这些限制，特别是不要依赖于 ARCH 的设计，不要依赖特定的寄存器呢，那么本文介绍的基于 Call Frame Information 的方法就是用来解决以上难题。

## 2.1. DWARF 定义的 section `.debug_frame`

出于方便调试器调试的需要，Unix 世界制定了 DWARF (Debugging With Attributed Record Formats) 标准，规范中包含了用于栈回溯的 (Call Frame Infomation, 简称 CFI)。DWARF 定义了许多编译器和调试器用来支持源码级调试的调试文件格式，其解决了许多编程语言 (如 C, C++,Fortran) 的需求并可以扩展到其他语言。DWARF 独立于体系结构，适用于任何处理器或操作系统，广泛应用于 Unix, Linux 和其他操作系统中。DWARF 的官网网址在 <https://dwarfstd.org/>。目前最新可用的 DWARF 已经到了第 5 个版本，而 CFI 的格式定义在 6.4 章节。有关 DWARF 的更多介绍，可以读一下 [这篇介绍][11]。

从实际操作上来说就是我们在编译时开启 `-g` 选项即可以生成 DWARF 的这些调试信息，这些信息都存储在 `.debug_` 开头的 section 中，随便找个例子可以如下试试：

```
$ riscv64-unknown-linux-gnu-gcc -g main.c
$ riscv64-unknown-linux-gnu-readelf -SW a.out | grep "debug"
  [24] .debug_aranges    PROGBITS        0000000000000000 0010a0 0000d0 00      0   0 16
  [25] .debug_info       PROGBITS        0000000000000000 001170 000723 00      0   0  1
  [26] .debug_abbrev     PROGBITS        0000000000000000 001893 0002e4 00      0   0  1
  [27] .debug_line       PROGBITS        0000000000000000 001b77 0002af 00      0   0  1
  [28] .debug_frame      PROGBITS        0000000000000000 001e28 0000a8 00      0   0  8
  [29] .debug_str        PROGBITS        0000000000000000 001ed0 000527 01  MS  0   0  1
  [30] .debug_line_str   PROGBITS        0000000000000000 0023f7 0001c2 01  MS  0   0  1
  [31] .debug_loclists   PROGBITS        0000000000000000 0025b9 00012b 00      0   0  1
```

其中我们关心的用于调试的 CFI 信息就存放在 section `.debug_frame` 中，可以用于告知调试器对应每条指令 SP 等寄存器保存在 stack frame 中的什么位置（具体 CFI 的格式我们稍后分析）。这样我们就可以摆脱对 FP 的依赖，也不会在 prologue/epilogue 中消耗额外的指令周期保存 FP。

但需要注意的是采用 section `.debug_frame` 实现栈回溯并不完美，它存在以下问题：

- 不支持 “运行时” 的栈回溯：这些 `.debug_` 开头的 section 都是不可加载（load）的 section，也就是说执行程序时这些 section 并不会被加载到内存, 而且这些 section 可以被直接 strip 掉，这也符合 DWARF 的本意，即这些 section 仅用于程序调试。但正是因为这些 section 不能加载到内存，所以 section `.debug_frame` 无法被用来做运行时的 Stack Unwinding，譬如 C++ 中的异常处理。
- 此外 section `.debug_frame` 也没有包含源语言的信息。

## 2.2. LSB 定义的 section `.eh_frame`

针对 section `.debug_frame` 的以上不足，现代的 Linux 操作系统在其 [Linux Standard Base （简称 LSB）标准][8](目前最新版是 5.0）中定义了一个 section `.eh_frame` （以及一个附加辅助的 section `.eh_framehdr`）来解决上述的难题。具体参考 [LSB 5.0 规范的 Core specification 的 10.6 Exception Frames 章节][12] 描述，我们在下面章节中还会具体介绍。`.eh_frame` 这个 section 的内容格式和 `.debug_frame` 非常类似，稍有一些不同。section `.eh_frame` 在 section `.debug_frame` 的基础上做了扩充，解决了 section `.debug_frame` 没有解决的剩余难题。 期中，eh 应该是 exception handler 的缩写（我猜的）。

- section `.eh_frame` 具备 load 属性（`SHF_ALLOC`），随程序一起加载到内存中。strip 时不会被移除。
- 拥有源语言信息。通过 LSDA 等扩展字段。

## 2.3. `.eh_frame` vs `.debug_frame`

这里暂不展开，具体参考一篇讨论 ["eh_frame or debug_frame"][10], 摘录部分文字如下：

> Ideally, eh_frame will be the minimal unwind instructions necessary to unwind the stack when exceptions are thrown/caught.  eh_frame will not include unwind instructions for the prologue instructions or epilogue instructions -- because we can't throw an exception there, or have an exception thrown from a called function "below" us on the stack.  We call these unwind instructions "synchronous" because they only describe the unwind state from a small set of locations.

> debug_frame would describe how to unwind the stack at every instruction location.  Every instruction of the prologue and epilogue.  If the code is built without a frame pointer, then it would have unwind instructions at every place where the stack pointer is modified.  We describe these unwind instructions as "asynchronous" because they describe the unwind state at every instruction location.

以上是理想下的行为，但实际的编译器行为可能和我们理想上定义的不一致。

## 2.4. 编译选项对 section `.debug_frame` 和 `.eh_frame` 的影响

这里主要总结一下 riscv64-unknown-linux-gnu-gcc 的编译选项对生成的目标文件的 section `.debug_frame` 和 section `.eh_frame` 的影响，clang 类似。我这里使用的 riscv64-unknown-linux-gnu-gcc 版本是 12.1.0

- 只要有 `-g` 就产生 section `.debug_frame`

- `-fexceptions`: 这个选项的目的是通知编译器启用异常处理。除了生成异常处理所需的额外指令外。还会为所有函数生成 unwinding table 信息，即生成 section `.eh_frame`。如果您不指定此选项，GCC 默认为 C++ 这种需要异常处理的语言启用它，对不需要异常处理的，譬如 C 语言则默认禁用该选项。参考 [GCC 手册中对该编译选项的描述][9] 如下：

  > Enable exception handling. Generates extra code needed to propagate exceptions. For some targets, this implies GCC generates frame unwind information for all functions, which can produce significant data size overhead, although it does not affect execution. If you do not specify this option, GCC enables it by default for languages like C++ that normally require exception handling, and disables it for languages like C that do not normally require it. However, you may need to enable this option when compiling C code that needs to interoperate properly with exception handlers written in C++. You may also wish to disable this option if you are compiling older C++ programs that don’t use exception handling.

- `-funwind-tables`: 会生成 section `.eh_frame`，但和 `-fexceptions` 不同的是不会生成异常处理所需的额外指令。我们一般不在命令行中手动添加该选项。参考 [GCC 手册中对该编译选项的描述][9] 如下：

  > Similar to -fexceptions, except that it just generates any needed static data, but does not affect the generated code in any other way. You normally do not need to enable this option; instead, a language processor that needs this handling enables it on your behalf.

- `-fasynchronous-unwind-tables`: 和 `-funwind-tables` 类似，区别是 `-fasynchronous-unwind-tables` 中 CFI 的信息确保针对函数的每一条指令都是可以回溯的，也就是说对于那些 prologure 和 epilogue 中的编译器自己添加的指令也可以回溯，这样当一些异步事件发生时，最典型的譬如 signal，栈回溯都可以发生。

  > Generate unwind table in DWARF format, if supported by target machine. The table is exact at each instruction boundary, so it can be used for stack unwinding from asynchronous events (such as debugger or garbage collector).

此外相关的知识点还包括：

- `.debug_frame` 和 `.eh_frame` 这两个 section 是独立生成的，互不影响。
- riscv64-unknown-linux-gnu-gcc 不会默认加上 `-g`/`-fexceptions`/`-funwind-tables`/`-fasynchronous-unwind-tables`。
- `-fexceptions`/`-funwind-tables`/`-fasynchronous-unwind-tables` 这些编译选项有对应的禁止选项，譬如对应 `-fexceptions` 有 `-fno-exceptions`。

# 3. 基于 CFI 的栈回溯工作原理分析

**注意：本文对基于 CFI 的栈回溯原理分析将主要关注 `.eh_frame`。**

在看 CFI 定义的具体格式之前，我们来看看如果自己设计实现栈回溯该怎么做

我们的需求就是针对函数的每一行指令（考虑到调试时我们可以对每一条指令都设置断点 `si`）都可以拿到栈回溯的必要信息，回顾基于 FP 的栈回溯原理，我们能想到的最直接方法就是针对每一行指令，将 FP 的位置信息记录下来，而在一个函数执行过程中，我们可以以相对于 SP 的 offest 的方式记录 FP 的位置，因为对于 active subroutine 我们天然可以拿到当前的 SP，而一旦根据 SP + offset 方式计算出 active subroutine 的 FP 位置，我们又可以得到上一级 caller 的 SP（以 RISC-V 为例这两者指向同一个位置），然后依次递推回溯即可。

在 CFI 的术语中，FP 的位置被称为 Canonical Frame Address，或者简称 CFA。具体参考 DWARFv5 的 6.4 Call Frame Information 的定义，我摘抄如下，注：当我们在 DWARF 的语境下描述 stack frame 时，全部被替换为 “call frame”。

> An area of memory that is allocated on a stack called a “call frame.” The call frame
> is identified by an address on the stack. We refer to this address as the Canonical
> Frame Address or CFA. Typically, the CFA is defined to be the value of the stack
> pointer at the call site in the previous frame (which may be different from its value
> on entry to the current frame).

在栈回溯的过程中我们还需要知道在 stack frame 中保存的那些寄存器的值所在的位置，在知道 CFA 的基础上，我们从编译器的角度出发也很容易知道这些位置。

下面我们举个简单的例子：参考 [示例代码][13] 的 `foos.c`

```cpp
extern void unwind_by_backtrace() ;

void foo_3() {
    unwind_by_backtrace();
}

void foo_2() {
    foo_3();
}

void foo_1() {
    foo_2();
}

void foo_0() {
    foo_1();
}
```

我们采用 `riscv64-unknown-linux-gnu-gcc -c foos.c -fomit-frame-pointer -fasynchronous-unwind-tables` 对该文件进行编译，这里加上 `-fomit-frame-pointer` 确保我们不产生 FP，`-fasynchronous-unwind-tables` 则是为了生成 section `.eh_frame`。然后反汇编 `riscv64-unknown-linux-gnu-objdump -d foos.o`，每个函数都差不多，我们截取两个  `foo_3()` 和 `foo_2()`，这两个函数存在调用和被调用关系 `foo_2()`-> `foo_3()`。 

```cpp
0000000000000000 <foo_3>:
   0:   1141                    addi    sp,sp,-16
   2:   e406                    sd      ra,8(sp)
   4:   00000097                auipc   ra,0x0
   8:   000080e7                jalr    ra # 4 <foo_3+0x4>
   c:   0001                    nop
   e:   60a2                    ld      ra,8(sp)
  10:   0141                    addi    sp,sp,16
  12:   8082                    ret

0000000000000014 <foo_2>:
  14:   1141                    addi    sp,sp,-16
  16:   e406                    sd      ra,8(sp)
  18:   00000097                auipc   ra,0x0
  1c:   000080e7                jalr    ra # 18 <foo_2+0x4>
  20:   0001                    nop
  22:   60a2                    ld      ra,8(sp)
  24:   0141                    addi    sp,sp,16
  26:   8082                    ret
```

我们可以对每一个函数建立一张表，来记录整个函数执行过程中，对应每条指令，CFA 的值的计算规则以及 stack frame 中可能保存的寄存器的值所在的位置的计算规则。

CFA 的值的计算规则可以基于 SP，因为函数中针对每一条指令我们都可以得到当时的 SP 的值。更具体地来说，如果是断点发生在 active subroutine 中，则 SP 值就是断点对应的那条指令执行时处理器硬件的 SP 值，对于回溯的栈，譬如 active subroutine 的 caller，其 SP 值就等于 active subroutine 的 CFA，然后逐级递推即可。

对于栈中保存的寄存器的值，首先每个函数需要保存哪些寄存器，这个从编译器的角度我们是已知的，对于这里的例子，`foo_3()` 和 `foo_2()` 函数执行过程中都会保存 ra（注：但有时候也会存在无需保存寄存器的情况），同时为了获得 ra 寄存器在 stack frame 中保存的值的位置，我们可以相对于 CFA 进行计算，因为在当前函数执行过程中 CFA 是保持不变的。其他复杂的情况可能会保存更多的寄存器，这里不再赘述。

基于以上概念，我们可以针对每个函数一张表，表的行对应函数的每条指令的地址，第二列是该指令对应的 CFA 的计算规则，譬如 sp + 32，表示当该条指令被执行时，处理器的 SP 值往高地址方向偏移 32，既是该指令对应的 CFA 的值。此外还存在第三列，是函数中指令对应的栈中保存的 ra 的内容位置的计算规则。因为我们这里的例子比较简单，所有函数的表都是类似的，所以我们提供了 `foo_3()` 的表。

**表 1，foo_3() 的 Call Frame Information 描述表**

| 指令的地址 | CFA 的计算规则 | 在栈中保存的 ra 的内容位置的计算规则 |
|-----------|---------------|------------------------------------|
| 0x00      | 此时 CFA 和 SP 指向同一个位置，所以是 SP + 0   | ra 未被保存入 stack frame |
| 0x02      | 上一条指令将 SP 向低地址方向移动 16 个字节，由于 CFA 保持不变，所以 CFA 的计算公式修改为 SP + 16 | ra 未被保存入 stack frame | 
| 0x04      | 无变化，还是 SP + 16 | ra 被上一条指令备份在 stack frame，位置在 CFA - 8 |
| 0x08      | 无变化，还是 SP + 16 | 无变化，位置在 CFA - 8 |
| 0x0c      | 无变化，还是 SP + 16 | 无变化，位置在 CFA - 8 |
| 0x0e      | 无变化，还是 SP + 16 | 无变化，位置在 CFA - 8 |
| 0x10      | 无变化，还是 SP + 16 | ra 的值已被上一条指令恢复，stack frame 中保存的 ra 的值无效 |
| 0x12      | 上一条指令将 SP 向高地址方向移动 16 个字节，，由于 CFA 保持不变，所以 CFA 的计算公式修改为 SP + 0   | ra 未被保存入 stack frame |

有了这张表，假设在 0x04 这条指令处发生断点，即断点发生在 `foo_3()` 函数中，则我们可以根据 **表 1** 的第三行的第二列的计算公式 `SP + 16`，用当前处理器的 SP 值（记为 SP3）加上 16 计算出该条指令对应的 `foo_3()` 函数的 CFA（记为 CFA3），根据计算公式 `SP + 16` 计算出 `CFA3 = SP3 + 16`，即 `foo_3()` 函数 对应的 stack frame 的 BOTTOM 位置，这个值也是 `foo_2()` 函数对应的 SP 值（记为 SP2），即 `SP2 = CFA3 = SP3 + 16`。如果要栈回溯，根据 **表 1** 第三行第三列 RA 的计算公式计算出 `foo_3()` 的 stack frame 中保存返回地址的位置为 `CFA3 - 8`，得到返回地址就可以去 `foo_2()` 函数对应的表中去查相应的行了，依次类推就可以继续这个回溯过程 ......

这样我们无需依赖 FP 完成了一次栈回溯，但我们看到，这依赖于我们需要有类似 **表 1** 这样的信息。而 section `.eh_frame` 就是用来保存这些信息的，只不过保存这张表的格式比较特殊，另外我们观察 **表 1**，还会发现，其实又不少行的内容是相同的，譬如 0x04 ~ 0x0e，这意味着我们在实际保存时可以对信息进行压缩。

# 4. section `.eh_frame` 中的数据结构

我们来看一下 `.eh_frame` 的实际内容，使用 readelf 工具，带上 `-wf` 选项，这已经是解码后的展现了，实际都是二进制格式，我们就不研究了，感兴趣可以参考 [LSB 5.0 规范的 Core specification 的 10.6 Exception Frames 章节][12]。借助 hexdump 和对 ELF 文件格式的理解自己查看。

```
$ riscv64-unknown-linux-gnu-readelf -wf foos.o
Contents of the .eh_frame section:


00000000 0000000000000010 00000000 CIE
  Version:               3
  Augmentation:          "zR"
  Code alignment factor: 1
  Data alignment factor: -4
  Return address column: 1
  Augmentation data:     1b
  DW_CFA_def_cfa_register: r2 (sp)
  DW_CFA_nop

00000014 0000000000000018 00000018 FDE cie=00000000 pc=0000000000000000..0000000000000014
  DW_CFA_advance_loc: 2 to 0000000000000002
  DW_CFA_def_cfa_offset: 16
  DW_CFA_advance_loc: 2 to 0000000000000004
  DW_CFA_offset: r1 (ra) at cfa-8
  DW_CFA_advance_loc: 12 to 0000000000000010
  DW_CFA_restore: r1 (ra)
  DW_CFA_advance_loc: 2 to 0000000000000012
  DW_CFA_def_cfa_offset: 0

00000030 0000000000000018 00000034 FDE cie=00000000 pc=0000000000000014..0000000000000028
  DW_CFA_advance_loc: 2 to 0000000000000016
  DW_CFA_def_cfa_offset: 16
  DW_CFA_advance_loc: 2 to 0000000000000018
  DW_CFA_offset: r1 (ra) at cfa-8
  DW_CFA_advance_loc: 12 to 0000000000000024
  DW_CFA_restore: r1 (ra)
  DW_CFA_advance_loc: 2 to 0000000000000026
  DW_CFA_def_cfa_offset: 0

0000004c 0000000000000018 00000050 FDE cie=00000000 pc=0000000000000028..000000000000003c
  DW_CFA_advance_loc: 2 to 000000000000002a
  DW_CFA_def_cfa_offset: 16
  DW_CFA_advance_loc: 2 to 000000000000002c
  DW_CFA_offset: r1 (ra) at cfa-8
  DW_CFA_advance_loc: 12 to 0000000000000038
  DW_CFA_restore: r1 (ra)
  DW_CFA_advance_loc: 2 to 000000000000003a
  DW_CFA_def_cfa_offset: 0

00000068 000000000000001c 0000006c FDE cie=00000000 pc=000000000000003c..0000000000000050
  DW_CFA_advance_loc: 2 to 000000000000003e
  DW_CFA_def_cfa_offset: 16
  DW_CFA_advance_loc: 2 to 0000000000000040
  DW_CFA_offset: r1 (ra) at cfa-8
  DW_CFA_advance_loc: 12 to 000000000000004c
  DW_CFA_restore: r1 (ra)
  DW_CFA_advance_loc: 2 to 000000000000004e
  DW_CFA_def_cfa_offset: 0
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
  DW_CFA_nop
```

规范定义每个 section `.eh_frame` 包含 1 个或多个 Call Frame Information (简称 CFI)。每个 CFI 的构成是以一个 Common Information Entry (简称 CIE) 开始，后跟 1 个或多个 Frame Description Entry (简称 FDE)。数据排列如下：

```
| CIE | FDE 0 | FDE 1 （可选） | ...... |
```

从上面的例子我们可以看出，`foos.o` 的 section `.eh_frame` 中包含有一个 CFI，这个 CFI 中有一个 CIE，后面紧跟着 4 个 FDE。一个 FDE 对应一个函数，CIE 中记录的是多个 FDE 公用的信息。

CIE 结构主要由如下字段构成：

- Length: 一个 4 byte 长的无符号值, 代表此 CIE 结构体的长度(不包括 Length 字段自身所占的 4 个字节), 如果 Length 值为 0xffffffff, 则 CIE 结构体的第一项被替换为 Extended length 字段，这是一个 8 字节长度的字段，其低 32 位就是原来的 Length。上面显示 0000000000000010, 是按照 Extended length 形式显示的，如果直接看内存，则只占用了 4 byte。
- Extended length，可选项，如果存在则占 8 byte，值记录了 CIE 字段的大小 (不包括 Extended length 字段自身所占的 8 个字节)。
- CIE ID: 一个 4 byte 的值用来区分 CIE 和 FDE,对于 CIE 此值永远是 0x00000000。
- Version：代表 Frame information 的版本, 其应该为 1 或者 3，在 RISC-V 上我看到的是 3。
- Augmentation：记录此 CIE 和 CIE 相关的 FDE 的增强特性，其是一个大小写敏感的字符串，以 `0x00` 结尾。字符串由一个或多个字符组成，每个字符有不同的含义，我们看到的是 `z` 和 `R`。如果 Augment 存在,则 z 必须是第一个字符，这就是个魔数，其他字符具体看规范定义 10.6.1.1.1. Augmentation String Format
- Initial Instructions：相当于可以认为用于设置上面 **表 1** 的初始值。看上面的例子是类似 `DW_CFA_def_cfa_register: r2 (sp)` 和 `DW_CFA_nop` 这样的字符串， 具体含义见下面对 FDE 的描述。

FDE 结构的组成如下：

- length/Extended length: 和 CIE 中定义相同
- CIE pointer: 当前位置到其 CIE 起始位置的偏移，根据这个值可以判断出来当前 FDE 关联的 CIE。上面例子显示，每个 FDE 都有一个 `FDE cie=00000000`，这表明这些 FDE 都关联于一个 CIE，这个 CIE 的起始位置就是 0x00000000
- pc begin/pc range:  此 FDE 负责的地址范围，也就是该 FDE 对应的函数的地址范围。
- ......
- Call Frame Instrctions(CFI): 这里是一条条的 CFI 指令构成的一个指令序列，这些指令可以理解为用来描述如何构建类似于我们上面 **表 1** 和  **表 2** 描述的表格。具体可以参考 DWARFv5 的 6.4.2 Call Frame Instructions，这些指令主要分为以下几大类：
  - Row Creation Instructions: 本类的指令会确定一个新的 Location(PC) 值，相当于在表中创建新的一行 (Row)。典型指令有：DW_CFA_set_loc、DW_CFA_advance_loc 等
  - CFA Definition Instructions: 在 Location 相关指令创建一个新行 (Row)以后，本节相关指令来定义这一行的CFA 的计算规则。典型指令有 DW_CFA_def_cfa_register、DW_CFA_def_cfa_offset 等
  - Register Rule Instructions：本类指令确定一行 (Row) 恢复寄存器的规则。典型指令有 DW_CFA_undefined 等，规则的取值包括 undefined、same value 等，具体见 DWARFv5 的 6.4.1 Structure of Call Frame Information 中对 register rules 的描述。
  - Row State Instructions：提供了备份(stack)和恢复(retrieve)完整寄存器状态的能力
  - Padding Instruction：即 DW_CFA_nop，用作填充以保证 CIE 或 FDE 的大小字节对齐。
  - CFI Extensions：其他扩展指令

但根据以上描述的 CFI 在逻辑上构建一个类似于我们 **表 1** 那样的表是比较繁琐的。readelf 提供了一个选项，可以将其展现成我们期望的形式，换成 `-wF` 即可。在实际使用中也以这种方式为多。

```
$ riscv64-unknown-linux-gnu-readelf -wF foos.o
Contents of the .eh_frame section:


00000000 0000000000000010 00000000 CIE "zR" cf=1 df=-4 ra=1
   LOC           CFA      
0000000000000000 sp+0     

00000014 0000000000000018 00000018 FDE cie=00000000 pc=0000000000000000..0000000000000014
   LOC           CFA      ra    
0000000000000000 sp+0     u     
0000000000000002 sp+16    u     
0000000000000004 sp+16    c-8   
0000000000000010 sp+16    u     
0000000000000012 sp+0     u     

00000030 0000000000000018 00000034 FDE cie=00000000 pc=0000000000000014..0000000000000028
   LOC           CFA      ra    
0000000000000014 sp+0     u     
0000000000000016 sp+16    u     
0000000000000018 sp+16    c-8   
0000000000000024 sp+16    u     
0000000000000026 sp+0     u     

0000004c 0000000000000018 00000050 FDE cie=00000000 pc=0000000000000028..000000000000003c
   LOC           CFA      ra    
0000000000000028 sp+0     u     
000000000000002a sp+16    u     
000000000000002c sp+16    c-8   
0000000000000038 sp+16    u     
000000000000003a sp+0     u     

00000068 000000000000001c 0000006c FDE cie=00000000 pc=000000000000003c..0000000000000050
   LOC           CFA      ra    
000000000000003c sp+0     u     
000000000000003e sp+16    u     
0000000000000040 sp+16    c-8   
000000000000004c sp+16    u     
000000000000004e sp+0     u
```

观察这里的第一个 FDE，其 pc 范围为 0000000000000000..0000000000000014，正对应 `foo_3()`。和我们自己绘制的 **表 1** 进行对比，就比较好理解了。

# 5. CFI directives

具体编译器是如何生成 `section .eh_frame` 的呢？首先编译器输入源文件，对其编译，在生成汇编的过程中插入一种专为 CFI 设计的 directives。我们可以执行 `riscv64-unknown-linux-gnu-gcc -S foos.c -fomit-frame-pointer -fasynchronous-unwind-tables`，然后看一下生成的 `foos.s`

```cpp
$ cat foos.s
        .file   "foos.c"
        .option nopic
        .text
        .align  1
        .globl  foo_3
        .type   foo_3, @function
foo_3:
.LFB0:
        .cfi_startproc
        addi    sp,sp,-16
        .cfi_def_cfa_offset 16
        sd      ra,8(sp)
        .cfi_offset 1, -8
        call    unwind_by_backtrace
        nop
        ld      ra,8(sp)
        .cfi_restore 1
        addi    sp,sp,16
        .cfi_def_cfa_offset 0
        jr      ra
        .cfi_endproc
.LFE0:
        .size   foo_3, .-foo_3
        .align  1
        .globl  foo_2
        .type   foo_2, @function
foo_2:
......
```

我们看到在汇编指令中加载了许多 `.cfi_*` 的指令，这就是 GCC 定义的一组 directives，参考 [GAS 手册的 7.12 CFI directives][14]。编译器生成汇编指令后，汇编器解析 CFI directives 为源文件在 obj 文件中生成 section `.eh_frame`。Linker 收集 obj 中的 `.eh_frame` input sections 生成 output section `.eh_frame`。

如果我们直接编写汇编代码，则需要自己手工添加 CFI directives，否者堆栈回溯信息会出错。

# 6. 通过 glibc 的 backtrace 函数实现栈回溯的例子

在编程时有很多有用的库提供了栈回溯的函数。我这里简单介绍一下基于 Glibc 提供的 backtrace 函数，具体可以 `man 3 backtrace`

具体例子参考 [示例代码][13] 的 `backtrace.c`

其中 `backtrace()`, `backtrace_symbols()` 都是库函数，先获取到地址，然后解析出对应的 symbol。编译命令如下，注意 `-rdynamic` 是必须的，否则无法得到符号表的名字。

```
$ riscv64-unknown-linux-gnu-gcc test.c foos.c backtrace.c -fomit-frame-pointer -fasynchronous-unwind-tables -rdynamic -o a.out
```

用 qemu 尝试运行一下, 因为我们是动态链接，所以执行 qemu 时加一下 `-L` 选项，指定动态链接器所在的 sysroot 路径：
```
qemu-riscv64 -L $YOUR_SYSROOT a.out
0x10a7a:a.out(unwind_by_backtrace+0x78) [0x10a7a]
0x109ca:a.out(foo_3+0x8) [0x109ca]
0x109da:a.out(foo_2+0x8) [0x109da]
0x109ea:a.out(foo_1+0x8) [0x109ea]
0x109fa:a.out(foo_0+0x8) [0x109fa]
0x109b8:a.out(main+0x8) [0x109b8]
0x400085a70c:/lib/libc.so.6(__libc_start_main+0x86) [0x400085a70c]
0x1093c:a.out(_start+0x2c) [0x1093c]
```

这个回溯链中任何一个函数如果缺失了 CFI 信息都会导致栈回溯不完整。假设我们故意删除 `foo_2()` 的 CFI 信息，执行后的效果如下：

```
qemu-riscv64 -L $YOUR_SYSROOT a.out
0x10a7a:a.out(unwind_by_backtrace+0x78) [0x10a7a]
0x109ca:a.out(foo_3+0x8) [0x109ca]
0x109da:a.out(foo_2+0x8) [0x109da]
```

有关支持 stack unwind 的函数的总结，可以考虑另起一篇，这篇总结已经足够长了。


[1]: ./20220717-call-stack.md
[2]: ./20220719-stack-unwinding.md
[3]: https://maskray.me/blog/2020-11-08-stack-unwinding
[4]: https://bbs.pediy.com/thread-270936.htm
[5]: https://blog.csdn.net/pwl999/article/details/107569603
[6]: ./20220719-stackuw-fp.md
[7]: https://dwarfstd.org/
[8]: https://refspecs.linuxfoundation.org/lsb.shtml
[9]: https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html
[10]: https://lists.llvm.org/pipermail/lldb-dev/2014-October/005541.html
[11]: https://zhuanlan.zhihu.com/p/419908664
[12]: https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/ehframechpt.html
[13]: ./code/20220721-stackuw-cfi/
[14]: https://sourceware.org/binutils/docs-2.38/as/CFI-directives.html
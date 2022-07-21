![](./diagrams/linker-loader.png)

文章标题：**Stack Unwinding 之基于 Frame Pointer**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 参考](#1-参考)
- [2. 基于 Frame Pointer 实现栈回溯的工作原理](#2-基于-frame-pointer-实现栈回溯的工作原理)
- [3. fp 的优化相关总结](#3-fp-的优化相关总结)
    - [3.1. `-O1`](#31--o1)
    - [3.2. `-O2`](#32--o2)
- [4. 基于 FP 实现 Stack Unwinding 的优势和不足](#4-基于-fp-实现-stack-unwinding-的优势和不足)

<!-- /TOC -->

# 1. 参考

本文主要参考了如下内容：

- 【参考 1】[Call Stack (RISC-V)][1]
- 【参考 2】[Stack Unwinding - Overview][2]
- 【参考 3】[Stack unwinding][3]
- 【参考 4】[AARCH64平台的栈回溯][4]
- 【参考 5】[Unwind 栈回溯详解][5]


本文主要总结基于 Frame Pointer 实现栈回溯（Stack Unwinding）的工作原理，至于什么是 “Stack” 以及什么是 “Stack Unwinding”，请阅读我的另外两篇总结 [《Call Stack (RISC-V)》][1] 和 [《Stack Unwinding - Overview》][2]。

# 2. 基于 Frame Pointer 实现栈回溯的工作原理

我们先分析一下在 [《Call Stack (RISC-V)》][1] 中的例子中如果编译时指定了 `-fomit-frame-pointer`，也就是没有引入 FP 的情况下，是否可以实现栈回溯。下图是没有 FP 的情况的 call stack。

```
          |        |
     ? -->+--------+ 
          |  ra'   | \       high address
          | ...... |  |
          |        |  caller
          |        |  |
          |        | / 
     ? -->+--------+
          |  ra    | \
          | ...... |  |
          |        |  callee <- active subroutine
     SP-->|--------|  | 
          |        | /       low address
```

如果要回溯，也就是要从上图中的 active subroutine 中找到 caller 的 stack frame 的位置（包括大小），但关键问题是每个函数的 call stack 的大小并不是固定的，所以当我们程序执行到 active subroutine 时，虽然我们有 SP 指向当前 stack frame 的 TOP，却没办法知道当前 stack frame 的 BOTTOM（ 即 caller 的 SP），也无从得知 caller 的 stack frame 的 BOTTOM。

我们想到的方法就是新引入一个寄存器 FP（以 RISC-V 为例，这个寄存器是 x8，其 ABI name 是 s0 或者 fp），和 SP 一起协同，指向当前 stack frame 的 BOTTOM，同时也是 caller 对应 stack frame 的 TOP，同时我们再在当前的 stack frame 里保存一下 caller 的 stack frame 的 BOTTOM 的位置（下图中的 fp，其值指向 caller 作为 active subroutine 时 FP 的值 FP'），如下所示。

```
 :                |        |
 |   +---->FP''-->+--------+ 
 |   |            |  ra''  | \       high address
 +---+---+--------+--fp''  |  |
     |            | ...... |  caller's caller
     |            |        |  |
     |            |        | / 
     |   +->FP'-->+--------+ 
     |   |        |  ra'   | \
     +---+--------+--fp'   |  |
         |        | ...... |  caller
         |        |        |  |
         |        |        | / 
         |   FP-->+--------+
         |        |  ra    | \
         +--------+--fp    |  |
                  | ...... |  callee <- active subroutine
             SP-->|--------|  | 
                  |        | /       low address
```

如果建立起这种联系，相当于在 call stack 中以 stack frame 为节点（node）建立起一个单向链表，每个 stack frame 中保存的 fp 值相当于就是链表节点中的单向指针，指向上一个节点。而访问 stack frame 中的 fp 非常方便，因为对于 RISC-V 这种 ARCH 来说，就是 FP 指针往低地址方向偏移固定的 offset，譬如在 RV64 上就是 16 个字节。同时我们知道基于 FP 向低地址方向偏移一个固定的 offset，譬如在 RV64 上 8 个字节就是存放的 caller 的 RA，那么我们也可以根据这个信息很容易拿到函数的信息。

为了实现这种数据结构，函数的 prologue 和 epilogure 指令部分会有相应的处理，这里就不赘述了，可以参考 [《Call Stack (RISC-V)》][1] 中 `-O0` 情况下的反汇编例子描述。

# 3. fp 的优化相关总结

在 [《Call Stack (RISC-V)》][1] 中我们在编译 `callstack.c` 时，没有带任何优化选项，默认为 `-O0`，但是如果使用优化选项，gcc 会做如下优化。

未优化的反汇编指令序列例子请大家参考 [《Call Stack (RISC-V)》][1] 中的演示，这里就不重复罗列了。

具体的代码例子参考 [这里][6]。

## 3.1. `-O1`

`-O1` 的结果如下，我们发现 fp 被优化掉了，相当于自动加上了 `-fomit-frame-pointer`（当然还包括了一些其他的优化，感兴趣可以自行对比）。

```cpp
0000000000000000 <foo>:
extern int bar(int a, int b);

void foo(void)
{
   0:   1141                    addi    sp,sp,-16
   2:   e406                    sd      ra,8(sp)
    int a = 1;
    int b = 2;
    bar(a, b);
   4:   4589                    li      a1,2
   6:   4505                    li      a0,1
   8:   00000097                auipc   ra,0x0
   c:   000080e7                jalr    ra # 8 <foo+0x8>

0000000000000010 <.LVL1>:
}
  10:   60a2                    ld      ra,8(sp)
  12:   0141                    addi    sp,sp,16
  14:   8082                    ret
```

如果在 `-O1` 情况下希望在 call stack 中保留 frame pointer，可以加上 `-fno-omit-frame-pointer` 告诉编译器不要把 frame pointer 优化掉

`-O1 -fno-omit-frame-pointer` 的结果如下:

```cpp
0000000000000000 <foo>:
extern int bar(int a, int b);

void foo(void)
{
   0:   1141                    addi    sp,sp,-16
   2:   e406                    sd      ra,8(sp)
   4:   e022                    sd      s0,0(sp)
   6:   0800                    addi    s0,sp,16
    int a = 1;
    int b = 2;
    bar(a, b);
   8:   4589                    li      a1,2
   a:   4505                    li      a0,1
   c:   00000097                auipc   ra,0x0
  10:   000080e7                jalr    ra # c <foo+0xc>

0000000000000014 <.LVL1>:
}
  14:   60a2                    ld      ra,8(sp)
  16:   6402                    ld      s0,0(sp)
  18:   0141                    addi    sp,sp,16
  1a:   8082                    ret
```

## 3.2. `-O2`

`-O2` 的结果如下，我们发现不仅 fp 被优化掉了，ra 都被优化掉了，这主要时因为我们这里的例子代码是 “尾调用”，所以会被优化到这个程度，而且 `jalr` + `ret` 的组合被优化成了一条 `jr`。

```cpp
0000000000000000 <foo>:

void foo(void)
{
    int a = 1;
    int b = 2;
    bar(a, b);
   0:   4589                    li      a1,2
   2:   4505                    li      a0,1
   4:   00000317                auipc   t1,0x0
   8:   00030067                jr      t1 # 4 <foo+0x4>
```

但注意在 `-O2` 的情况下，光加上 `-fno-omit-frame-pointer` 还不足以实现正常的栈回溯，见下面 `-O2 -fno-omit-frame-pointer` 的反汇编结果，line 6 在调用 `bar()` 函数之前过早地恢复了 fp，这对栈回溯是有影响的。

```cpp
0000000000000000 <foo>:
extern int bar(int a, int b);

void foo(void)
{
   0:   1141                    addi    sp,sp,-16
   2:   e422                    sd      s0,8(sp)
   4:   0800                    addi    s0,sp,16
    int a = 1;
    int b = 2;
    bar(a, b);
}
   6:   6422                    ld      s0,8(sp)
    bar(a, b);
   8:   4589                    li      a1,2
   a:   4505                    li      a0,1
}
   c:   0141                    addi    sp,sp,16
    bar(a, b);
   e:   00000317                auipc   t1,0x0
  12:   00030067                jr      t1 # e <foo+0xe>
```

所以如果考虑到尾调用的情况，还要加上 `-fno-optimize-sibling-calls` 选项，该选项让编译器不要优化尾部调用。`-O2 -fno-omit-frame-pointer -fno-optimize-sibling-calls` 的结果如下，这样就没有问题了。

```cpp
0000000000000000 <foo>:
extern int bar(int a, int b);

void foo(void)
{
   0:   1141                    addi    sp,sp,-16
   2:   e022                    sd      s0,0(sp)
   4:   e406                    sd      ra,8(sp)
   6:   0800                    addi    s0,sp,16
    int a = 1;
    int b = 2;
    bar(a, b);
   8:   4589                    li      a1,2
   a:   4505                    li      a0,1
   c:   00000097                auipc   ra,0x0
  10:   000080e7                jalr    ra # c <foo+0xc>

0000000000000014 <.LVL1>:
}
  14:   60a2                    ld      ra,8(sp)
  16:   6402                    ld      s0,0(sp)
  18:   0141                    addi    sp,sp,16
  1a:   8082                    ret
```

# 4. 基于 FP 实现 Stack Unwinding 的优势和不足

基于这种方式实现 Stack Unwinding 时非常方便快捷。但是这种方法也有自己的不足：

- 需要一个专门寄存器（在 RISC-V 中这个寄存器是 x8，其 ABI name 是 s0 或者 fp ）来保存 frame poniter。
- 在函数的 prologue 和 epilogue 中会有额外的 push fp 寄存器或者 pop fp 寄存器信息的动作，实际执行时增大了指令开销。
- 在栈回溯过程中，除了恢复 sp（也包括 fp） 和 ra，并不能恢复其他的寄存器。
- 依赖于 ARCH 和编译器的行为，譬如某种 ARCH 因为寄存器不够用无法专门预留一个 fp 寄存器，或者编译函数时使用了 `-O1` 及以上的优化。实践中也不能保证所有库都包含 frame pointer。
- 没有源语言信息。

[1]: ./20220717-call-stack.md
[2]: ./20220719-stack-unwinding.md
[3]: https://maskray.me/blog/2020-11-08-stack-unwinding
[4]: https://bbs.pediy.com/thread-270936.htm
[5]: https://blog.csdn.net/pwl999/article/details/107569603
[6]: ./code/20220717-call-stack/


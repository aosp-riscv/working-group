![](./diagrams/android-riscv.png)

文章标题：**AOSP RISC-V 移植工作中 setjmp 相关函数实现总结**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

历史记录：

- 2023/4/16，AOSP 代码部分修改为采用 Google upstream

文章大纲

<!-- TOC -->

- [1. setjmp 函数的使用](#1-setjmp-函数的使用)
- [2. `setjmp()` 函数实现 "nonlocal gotos" 的原理分析](#2-setjmp-函数实现-nonlocal-gotos-的原理分析)
- [3. AOSP 中针对 RISC-V 的 setjmp 实现](#3-aosp-中针对-risc-v-的-setjmp-实现)

<!-- /TOC -->

# 1. setjmp 函数的使用

直接 `man 3 setjmp` 会看到 POSIX 标准中的相关说明。

man 中我们看到实际上是一组函数，一共四个，分为两组，为简单起见，本文统称它们为 setjmp。

我们先看一下函数的原型：

```cpp
int setjmp(jmp_buf env);
int sigsetjmp(sigjmp_buf env, int savesigs);

void longjmp(jmp_buf env, int val);
void siglongjmp(sigjmp_buf env, int val);
```

`setjmp()` 和 `sigsetjmp()` 实现类似的功能，`longjmp()` 和 `siglongjmp()` 实现类似的功能。从使用上又可以分为以下两组：

```cpp
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

int sigsetjmp(sigjmp_buf env, int savesigs);
void siglongjmp(sigjmp_buf env, int val);
```

也就是说我们通常把 `setjmp()` 和 `longjmp()` 配合起来用，把 `sigsetjmp()` 和 `siglongjmp()` 配合起来用。

`sigsetjmp()/siglongjmp()` 这一对函数实现的功能和 `setjmp()/longjmp()` 类似，唯一的区别是 `sigsetjmp()` 相比 `setjmp()` 多一个参数 `savesigs`，允许我们指定在保存当前执行环境时是否需要保存 signal mask。这也说明对于 `setjmp()`, signal mask 的信息是默认保存的。


下面结合一个例子理解一下这些函数的用途，我们以 `setjmp()` 和 `longjmp()` 这一对为例。

```cpp
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

jmp_buf env;

int divide(int a, int b) {
    if (b == 0) {
        printf("catching ...: divided by zero\n");
        longjmp(env, 100);
        printf("will not reach here!!!\n");
    }
    return a / b;
}

int main(int argc, char const *argv[]) {
    int res = setjmp(env);
    if (res == 0) {
        printf("return from setjmp\n");
        divide(10, 0);
    } else {
        printf("return from longjmp: %d\n", res);
    }
    return 0;
}
```

输出结果为：

```shell
return from setjmp
catching ...: divided by zero
return from longjmp: 100
```

解释一下程序的执行：

- 首先调用 `setjmp()` 保存当前执行环境 (calling environment) 到 `env`，然后返回 0。此时我们看到打印输出 "return from setjmp"
- 程序继续执行 `divide()`，因为我们传入的除数 `b` 为 0，所以打印输出 "catching ...: divided by zero"，然后调用 `longjmp()`，传入上面保存的 `env`，以及另一个值 100。
- 此时 "诡异" 的一幕开始出现，调用 `longjmp()` 函数后面的打印 "will not reach here!!!" 不会出现，我们看到的打印输出是 "return from longjmp: 100"。从 `main()` 函数的代码来看，程序貌似又一次（代替我们自动）调用了 `setjmp()` 函数而且返回的值正是我们传入 `longjmp()` 的第二个参数的值。或者换句话说，程序的执行逻辑从 `longjmp()` 函数里面直接跳转到 `main()` 函数中调用 `setjmp()` 的位置并再一次从 `setjmp()` 函数中返回了。

这种和我们平日里常见的跳转行为很不一样的行为，在 man 手册中称其为 `"nonlocal gotos": transferring execution from one function to a predetermined location in another function.`。我们中文世界常称其为 “非本地跳转”，它可以将控制流直接从一个函数（在本文例子里是 divide()）转移到另一个函数（在本文例子里是 main()），而不需要经过正常的调用和返回序列，而且这种函数间跳转和 goto 语句实现的跳转也不一样，goto 语句只能实现本函数内的任意位置跳转，所以 man 手册中给这里的定义叫 "nonlocal gotos"，大家可以自己体会一下区别。

顺便说一下，上面这个例子有点类似 C++ 或 Java 的异常处理机制，但 C 语言没有 C++ 或 Java 的异常机制，但我们可以通过 `setjmp()/longjmp()` 这一对函数实现类似的效果。

# 2. `setjmp()` 函数实现 "nonlocal gotos" 的原理分析

man 手册上的说明详细地解释了 `setjmp()/longjmp()` 行为，摘录如下：

> The setjmp() function saves various information about the calling environment (typically, the stack pointer, the instruction pointer, possibly the values of other registers  and the signal mask) in the buffer env for later use by longjmp().  In this case, setjmp() returns 0.
> 
> The  longjmp() function uses the information saved in env to transfer control back to the point where setjmp() was called and to restore ("rewind") the stack to its state at the time of the setjmp() call. In addition, and depending on the implementation (see NOTES), the values of some other registers and the process signal mask may be restored to their state at the time of the setjmp() call.
>
> Following  a  successful longjmp(), execution continues as if setjmp() had returned for a second time.  This "fake" return can be distinguished from a true setjmp() call because  the "fake" return returns the value provided in val.  If the programmer mistakenly passes the value 0 in val, the "fake" return will instead return 1.

所以要理解其行为，首先需要深刻理解的概念是所谓的 "calling environment"。man 手册中解释了，这些调用信息包括一些处理器的寄存器信息，譬如 stack pointer, instruction pointer（即我们常说的 PC）以及 signal mask 等。

`setjmp()` 第一次被调用时的主要行为就是将当前处理器的这些寄存器和 signal mask 的信息保存到 `env` 所指向的内存中，而且这块内存的生命周期必须要保证后面的 `longjmp()` 函数可以访问到，在上面的例子中，这个 `env` 内存被定义为一个全局变量。

当 `longjmp()` 函数被调用时，它要做的事情就是利用 `setjmp()` 当初保存在 env 中的处理器上下文信息将执行状态恢复到 `setjmp()` 第一次被调用时的状态，这包括了 PC 值、栈状态以及其他信息。恢复了以后再继续执行其实从 CPU 的角度来看就是又回到了 `setjmp()` 被第一次调用的地方，这就是 man 手册中说的 `as if setjmp() had returned for a second time.`。但为了和第一次返回有所区别，第一次默认成功返回 0，而第二次默认成功返回 `longjmp()` 的第二个参数的值。

# 3. AOSP 中针对 RISC-V 的 setjmp 实现

理解了上一节的原理，我们要做的其实就是针对 riscv 实现 `setjmp()` 和 `longjmp()` 的内部逻辑。

具体参考 [`libc/arch-riscv64/bionic/setjmp.S`][1]

首先我们要定义一下 env 这块 buffer 中的内容安排，这个 env 对应的内存内容实际上对于 setjmp/longjmp 的使用者来说是一个黑盒子，我们可以自己定义其中具体的安排， 参考代码中的注释，这些涉及的寄存器信息就是针对 RISC-V 我们关心的 "calling environment"。特别地 RISC-V 中没有 pc 寄存器，但是对应的有 ra，它保存了返回地址。stack pointer 我们有 sp ......

```cpp
// The internal structure of a jmp_buf is totally private.
// Current layout (changes from release to release):
//
// word   name            description
// 0      sigflag/cookie  setjmp cookie in top 31 bits, signal mask flag in low bit
// 1      sigmask         64-bit signal mask
// 2      ra
// 3      sp
// 4      gp
// 5      s0
// ......
// 16     s11
// 17     fs0
// ......
// 28     fs11
// 29     checksum
// _JBLEN: defined in bionic/libc/include/setjmp.h
```

而 `jmp_buf` 的结构体定义可以参阅 [`libc/include/setjmp.h`][2]
<https://github.com/riscv-android-src/platform-bionic/blob/riscv64-android-12.0.0_dev/libc/include/setjmp.h>

```cpp

#if defined(__aarch64__)
......
#elif defined(__riscv)
#define _JBLEN 32
#endif

typedef long sigjmp_buf[_JBLEN + 1];
typedef long jmp_buf[_JBLEN];
```

具体的代码我们就不在这里解释了，理解了原理，代码其实比较简单。



[1]:https://github.com/aosp-mirror/platform_bionic/blob/7dd3896fe1ec5160169b17507962fc699abea39f/libc/arch-riscv64/bionic/setjmp.S
[2]:https://github.com/aosp-mirror/platform_bionic/blob/7dd3896fe1ec5160169b17507962fc699abea39f/libc/include/setjmp.h
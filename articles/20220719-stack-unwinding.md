![](./diagrams/linker-loader.png)

文章标题：**Stack Unwinding - Overview**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 参考](#1-参考)
- [2. 什么是 Stack Unwinding](#2-什么是-stack-unwinding)
- [3. 哪些场景下需要 Stack Unwinding](#3-哪些场景下需要-stack-unwinding)
- [4. 实现 Stack Unwinding 的方法](#4-实现-stack-unwinding-的方法)

<!-- /TOC -->

最近一直在研究和学习有关栈回溯的问题，由于内容比较多，所以一边看一边做笔记，同时整理一下自己的思路。

# 1. 参考

本文主要参考了如下内容：

- 【参考 1】[Call Stack (RISC-V)][1]
- 【参考 2】[Stack unwinding][2]
- 【参考 3】[Deep Wizardry: Stack Unwinding][3]
- 【参考 4】[Stack Unwinding in C++][4]


# 2. 什么是 Stack Unwinding

在 [【参考 1】][1] 中我们了解到在计算机程序执行过程中，当 caller 函数调用 callee 函数时，程序会在 call stack 中为 callee 函数分配一块新的 stack frame，同时 stack pointer 也指向了 callee 函数（即 active subroutine）。[【参考 1】][1] 中的 【图一】到【图四】中 SP 的向下移动的过程我们形象地称其为 “Winding”。那么与之相反的过程自然就是 “Unwinding” 了。因为这个过程发生在 call stack 中，所以我们称其为 “Stack Unwinding”， 中文翻译为 “栈回溯”。

所以我们定义：在 call stack 中，从最近压栈的 callee 函数对应的 stack frame 开始向 caller 方向进行遍历的过程，称之为  “Stack Unwinding”。

值得注意的是，“Stack Unwinding” 这个过程的发生并不一定意味着函数一定要返回，在回溯过程中 call stack 可能是保持不变的。而且 “Stack Unwinding” 甚至可能并不一定发生在程序执行过程中，我们只要把它理解成一种逻辑上对 call stack 的信息进行反向遍历的过程行为就好了。

# 3. 哪些场景下需要 Stack Unwinding

最常见的栈回溯发生在我们使用调试器（Debugger）对程序进行调试时，譬如使用 gdb 中我们将程序指令 break 住以后，执行 `backtrace` 命令打印整个 call stack，call stack 中每一个 stack frame 一行。

```bash
(gdb) bt
#0  foo_3 () at test_backtrace.c:20
#1  0x00005555555548e6 in foo_2 () at test_backtrace.c:24
#2  0x00005555555548f7 in foo_1 () at test_backtrace.c:28
#3  0x0000555555554908 in foo () at test_backtrace.c:32
#4  0x0000555555554919 in main () at test_backtrace.c:37
(gdb) 
```

还有一种常见的栈回溯和类似 C++ 这种支持异常处理的程序语言有关。以下代码摘录自 [【参考 4】][4]

```cpp
// CPP Program to demonstrate Stack Unwinding
#include <iostream>
using namespace std;
  
// A sample function f1() that throws an int exception
void f1() throw(int)
{
    cout << "\n f1() Start ";
    throw 100;
    cout << "\n f1() End ";
}
  
// Another sample function f2() that calls f1()
void f2() throw(int)
{
    cout << "\n f2() Start ";
    f1();
    cout << "\n f2() End ";
}
  
// Another sample function f3() that calls f2() and handles
// exception thrown by f1()
void f3()
{
    cout << "\n f3() Start ";
    try {
        f2();
    }
    catch (int i) {
        cout << "\n Caught Exception: " << i;
    }
    cout << "\n f3() End";
}
  
// Driver Code
int main()
{
    f3();
  
    getchar();
    return 0;
}
```
程序运行输出如下：

```bash
 f3() Start 
 f2() Start 
 f1() Start 
 Caught Exception: 100
 f3() End
```

函数的调用关系是 main->f3->f2->f1，当 f1 抛出异常时，由于 f1 没有定义异常处理函数，所以根据 C++ 的 ABI 定义（[【参考 5】][5]），将发生栈回溯，逐级回退并检查每一级函数是否可以处理该异常，直到回溯到 f3 执行完到异常处理并打印 `Caught Exception: 100`。

除了以上两种最常见的会涉及栈回溯的场景外，还有：
- 自己编写的程序中也可能会在 active subroutine 中尝试遍历当前 call stack 并执行一些自定义的动作。
- 一些工具分析软件中会遍历 call stack 的信息，这些分析可能发生在 online 状态，也可能发生在 offline 状态。

# 4. 实现 Stack Unwinding 的方法

一句话总结：有很多。但常见的方法有两种，内容比较多，我打算另辟两篇文章如下单独总结。

- [基于 Frame Pointer 的栈回溯][6]
- [基于 Call Frame Information 的栈回溯][7]

除此之外，还有一些基于以上方法的改良定制版本，以及基于 ARCH 自身自己发明的方法，我这里就不一一赘述了，感兴趣可以阅读 [【参考 2】][2]。


[1]: ./20220717-call-stack.md
[2]: https://maskray.me/blog/2020-11-08-stack-unwinding
[3]: https://blog.reverberate.org/2013/05/deep-wizardry-stack-unwinding.html
[4]: https://www.geeksforgeeks.org/stack-unwinding-in-c/
[5]: https://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html
[6]: ./20220719-stackuw-fp.md
[7]: ./20220721-stackuw-cfi.md
![](./diagrams/linker-loader.png)

文章标题：**用于栈回溯的一些库**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

在研究 Stack unwinding 的过程中接触到了不少用于该目的的库，总结在这里方便以后查看。

在这些库中，我目前主要研究的是 [nongnu 的 libunwind][1] 和 [LLVM 的 libunwind][2]，所以在这篇笔记中我也会把我对这两个项目的代码理解简单总结一下（先只列了 nongnu 的 libunwind，LLVM 的 libunwind 的实现后来看了一下实际类似，只是 llvm 的代码全部是 c++ 写的，比较抽象，可读性我觉得反而没有 nongun 的 libunwind 的 c 读起来舒服）。

后面如果有机会涉及 unwinding 的库都尽量总结在这里。

所有例子代码主要基于 RISC-V 的平台测试。

文章大纲

<!-- TOC -->

- [1. 参考](#1-参考)
- [2. 简单的罗列](#2-简单的罗列)
- [3. GCC/Clang Built-in Function](#3-gccclang-built-in-function)
- [4. glibc 提供的 `backtrace`/`backtrace_symbols` 函数](#4-glibc-提供的-backtracebacktrace_symbols-函数)
- [5. libunwind (nongnu)](#5-libunwind-nongnu)
	- [5.1. 构建 libunwind (nongnu)](#51-构建-libunwind-nongnu)
	- [5.2. 使用 libunwind (nongnu)](#52-使用-libunwind-nongnu)
	- [5.3. libunwind (nongnu) 代码分析](#53-libunwind-nongnu-代码分析)

<!-- /TOC -->

# 1. 参考

- 【参考 1】[The libunwind project (nongnu)][1]
- 【参考 2】[LLVM libunwind][2]
- 【参考 3】[Assembling a Complete Toolchain 之 Unwind library][3]

# 2. 简单的罗列

有关什么是 “Stack” 以及什么是 栈回溯 “Stack Unwinding”，请阅读我的另外两篇总结 [《Call Stack (RISC-V)》][1] 和 [《Stack Unwinding - Overview》][2]。本会涉及的栈回溯还会涉及信号栈，相关技术背景分析见另一篇笔记 [《聊一聊 Linux 上信号处理过程中的信号栈帧》][6]。

市面上支持栈回溯的库有不少，自己总结了一下，列在这里备忘，主要参考了 [【参考 3】] 和网上搜索的结果：

- GCC/Clang Builtin Function，例如 `__builtin_return_address`/`__builtin_frame_address`: 非常粗糙的底层的实现，简单用用可以。函数说明参考 [GCC 手册第 6.51 章节 Getting the Return or Frame Address of a Function][9]，Clang 支持的 Builtin 函数的定义同 GCC。
- glibc 提供的 `backtrace`/`backtrace_symbols` 函数，具体使用可以 `man 3 backtrace`
- libunwind (nongnu): 这个 libunwind 是 HP libunwind 的继承者。实现了相当完整的 unwinding 的功能，具体介绍参考其 [官网][1]。
- libunwind (llvm)，LLVM 内置 的 unwind 库，使用 clang 编译程序时可以直接使用，这样就不需要依赖其他的外部 unwind 库了（自然也不依赖 GNU 的 unwinding 实现，见上一条）。具体介绍见 [llvm 文档][3]。
- libunwind (PathScale)，pathscale/libunwind。没啥研究，官网仓库地址看 [这里][10]（注意这个地址 404 了）。
- libgcc/libgcc_s (GNU)，GCC 内置的 unwind 库，使用 gcc 编译程序时可以直接使用，这样就不需要依赖其他的外部 unwind 库了。参考 [The GCC low-level runtime library 的 Exception handling routines][7] 和 [LSB: Interfaces for libgcc_s][8]。
- Android 上提供的一些 unwinding 库，还没仔细总结过，以后再补充或者另起一篇总结。

# 3. GCC/Clang Built-in Function

例如 `__builtin_return_address`/`__builtin_frame_address`: 非常粗糙的底层的实现，简单用用可以。函数说明参考 [GCC 手册第 6.51 章节 Getting the Return or Frame Address of a Function][9]。

这两个函数的原型如下：
```cpp
void * __builtin_return_address (unsigned int level);
void * __builtin_frame_address (unsigned int level);
```

其中 `__builtin_return_address` 函数可以获得 call stack 中对应函数的 RA，所谓对应函数得定义由 level 参数指定，0 对应 active subroutine，1 对应 active subroutine 的 caller，依此类推。`__builtin_frame_address` 函数可以获得 call stack 中对应函数的 FP，level 的含义同 `__builtin_return_address`。需要注意的是 GCC 手册上给出了如下说明：

> Calling this function with a nonzero argument can have unpredictable effects, including crashing the calling program. As a result, calls that are considered unsafe are diagnosed when the -Wframe-address option is in effect. Such calls should only be made in debugging situations.

如果我们这么做，如果编译时带上 `-Wall`，编译器也会生成相应的告警，譬如：`warning: calling '__builtin_return_address' with a nonzero argument is unsafe [-Wframe-address]`。也就是说 level 取 0 是安全的，其他的值则不安全（不保证正确），这和 ARCH 的实现和支持有关，以及对应函数是否是 tail call 或者由于 inline 导致 unwinding 所需要的信息丢失。毕竟这是编译器的 builtin 函数，其行为和 ARCH 紧密相关，不保证可移植性，所以我们简单用用（调试）就好了，正式的做法还是利用一些标准库譬如 glibc 中的 backtrace 或者 libunwind 来做。

另外我在测试时发现，如果我调用了 `__builtin_frame_address` 函数，即使我编译时指定 `-fomit-frame-pointer`，编译器依然会为我生成 fp 的相关指令，所以我怀疑这个 builtin 函数的实现是非常依赖 fp 的，但这仅仅在 RISC-V 上，其他的 ARCH 我暂时没有研究。

具体的例子请大家直接看 [test_builtin.c][11] 就好了。

# 4. glibc 提供的 `backtrace`/`backtrace_symbols` 函数

这些函数在我们演示基于 CFI 的栈回溯例子中被大量使用，可以看一下具体的例子: [backtrace.c][12]。

具体使用可以 `man 3 backtrace`。使用时有以下注意事项：

- inline 关键字描述的函数没有栈帧 (stack frames)
- 尾调用 (Tail-call) 优化会造成一个栈帧被另一个替换掉
- 对于使用 GNU linker 的系统，使用 `-rdynamic` 链接项，否则可能无法获取函数名字。并且 `Note that  names  of  "static" functions are not exposed, and won't be available in the backtrace.`（摘录自 man 手册）

# 5. libunwind (nongnu)

这个 libunwind 是 HP libunwind 的继承者。实现了相当完整的 unwinding 的功能。可以用于实现如下功能，摘录自其 [官网][1] 的原话：

> - exception handling
>   The libunwind API makes it trivial to implement the stack-manipulation aspects of exception handling.
> - debuggers
>   The libunwind API makes it trivial for debuggers to generate the call-chain (backtrace) of the threads in a running program.
> - introspection
>   It is often useful for a running thread to determine its call-chain. For example, this is useful to display error messages (to show how the error came about) and for performance monitoring/analysis.
> - efficient setjmp()
>   With libunwind, it is possible to implement an extremely efficient version of setjmp(). Effectively, the only context that needs to be saved consists of the stack-pointer(s).

这个库定义了一套 C 形式的 API，而且是可移植的，支持多种 ARCH 和 平台。API 分为两类：

- 第一类，称为 low level API，这套接口以 `unw_` 作为函数名前缀，譬如 `unw_getcontext` / `unw_init_local`, 这套 API 继承自早期的 HP libunwind。
- 第二类，称为 high level API，这套接口以 `__Unwind_` 作为函数名前缀，定义在 [unwind.h][13] 中，high level API 又可以分为两部分，一部分是 [Itanium C++ ABI 定义的 Exception Handling 接口中的 Base ABI][14]，还有一部分是 [LSB 要求实现的 Unwind API][8]。在 libunwind (nongnu) 中 high level API 实际上是对 low level API 的封装。

## 5.1. 构建 libunwind (nongnu)

自己从源码编译一个 libunwind 库，先从 github 仓库下载源码：

```bash
$ git clone git@github.com:libunwind/libunwind.git
$ cd libunwind
```

然后切换到你关心的 commit 上去，以 RISC-V 为例, 注：libunwind (nongnu) 从 1.6.0 开始才支持 riscv。我这里直接用的最新的 master。

发现直接构会在编译链接 test-coredump-unwind 时报错，具体看 <https://github.com/libunwind/libunwind/issues/395>，可能与我这里使用了自己构建的 riscv64-unknown-linux-gnu-gcc 有关，但我暂时还没有去定位原因，为了继续测试，我简单将制作 test-coredump-unwind 这个测试程序的步骤屏蔽了，应该不影响 libunwind 库的生成。

```
diff --git a/tests/Makefile.am b/tests/Makefile.am
index fa867367..3a6abb99 100644
--- a/tests/Makefile.am
+++ b/tests/Makefile.am
@@ -77,7 +77,7 @@ endif
 if OS_LINUX
 if BUILD_COREDUMP
  check_SCRIPTS_cdep += run-coredump-unwind
- noinst_PROGRAMS_cdep += crasher test-coredump-unwind
+# noinst_PROGRAMS_cdep += crasher test-coredump-unwind
 
 if HAVE_LZMA
  check_SCRIPTS_cdep += run-coredump-unwind-mdi
@@ -239,9 +239,9 @@ Ltest_mem_validate_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
 test_setjmp_LDADD = $(LIBUNWIND_setjmp)
 ia64_test_setjmp_LDADD = $(LIBUNWIND_setjmp)
 
-if BUILD_COREDUMP
-test_coredump_unwind_LDADD = $(LIBUNWIND_coredump) $(LIBUNWIND) @BACKTRACELIB@
-endif
+# if BUILD_COREDUMP
+# test_coredump_unwind_LDADD = $(LIBUNWIND_coredump) $(LIBUNWIND) @BACKTRACELIB@
+# endif
 
 Gia64_test_nat_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
 Gia64_test_stack_LDADD = $(LIBUNWIND) $(LIBUNWIND_local)
```

然后运行 autoreconf 生成 configure， 下面就没有什么特别需要关注的了，我简单列举构建过程如下：

```bash
$ cd libunwind
$ autoreconf -i
$ BUILD=x86_64-linux-gnu
$ cd ..
$ mkdir build
$ cd build/
$ ../libunwind/configure --prefix=<PATH_WHERE_YOU_WANT_TO_INSTALL> --build=x86_64-linux-gnu --host=riscv64-unknown-linux-gnu
$ make
$ make install
```

## 5.2. 使用 libunwind (nongnu)

写一个简单的应用程序测试一下 libunwind 的 stack unwinding 功能，参考例子代码 [libunwind.c][15] 和 [test_libunwind.c][16]

编译命令行如下：

```bash
$ riscv64-unknown-linux-gnu-gcc -fomit-frame-pointer -fexceptions -I ${PATH_LIBUNWIND_INCLUDE} -L ${PATH_LIBUNWIND_LIB} -lunwind-riscv -lunwind -Wall -o a.out libunwind.c test_libunwind.c
```

编译完后使用 qemu 运行结果如下：

```bash
$ qemu-riscv64 qemu-riscv64 -L ${PATH_SYSROOT} -E LD_LIBRARY_PATH=${PATH_LIBUNWIND_LIB} a.out
ip = 108a4, sp = 40007ffdf0 : (foo_3 + 0x00000008)
ip = 108b4, sp = 40007ffe00 : (foo_2 + 0x00000008)
ip = 108c4, sp = 40007ffe10 : (foo_1 + 0x00000008)
ip = 108d4, sp = 40007ffe20 : (foo + 0x00000008)
ip = 108e4, sp = 40007ffe30 : (main + 0x00000008)
ip = 40008a770c, sp = 40007ffe40 : (__libc_start_main + 0x00000086)
ip = 106ac, sp = 40007fffb0 : (_start + 0x0000002c)
```

再测试一下涉及 signal frame 的 stack unwinding， 参考例子代码 [libunwind.c][15] 和 [test_libunwind_signal.c][19]

注意运行时显示如下：

```bash
$ qemu-riscv64 qemu-riscv64 -L ${PATH_SYSROOT} -E LD_LIBRARY_PATH=${PATH_LIBUNWIND_LIB} a.out
ip = 10948, sp = 40007ff9d0 : (signal_handler + 0x0000000c)
ip = 400081f000, sp = 40007ff9f0 : (setitimer + 0x0000000c)
ip = 40008b648c, sp = 40007ffd30 : (gsignal + 0x00000092)
ip = 10966, sp = 40007ffe30 : (main + 0x00000016)
ip = 40008a770c, sp = 40007ffe40 : (__libc_start_main + 0x00000086)
ip = 1074c, sp = 40007fffb0 : (_start + 0x0000002c)
```

但打印的第二行 `ip = 400081f000, sp = 40007ff9f0 : (setitimer + 0x0000000c)` 这里显示的 setitimer 应该不准确，应该对应的是 `__vdso_rt_sigreturn`。具体什么原因造成的，还没有深入去看。

## 5.3. libunwind (nongnu) 代码分析

其他函数暂不赘述，主要看一下 RISC-V 的 [`unw_step` 函数][17], 这里取的是 v1.6.2 的版本：

```cpp
int
unw_step (unw_cursor_t *cursor)
{
  struct cursor *c = (struct cursor *) cursor;
  int validate = c->validate;
  int ret;

  Debug (1, "(cursor=%p, ip=0x%016lx, sp=0x%016lx)\n",
         c, c->dwarf.ip, c->dwarf.cfa);

  /* Validate all addresses before dereferencing. */
  c->validate = 1;

  /* Special handling the signal frame. */
  if (unw_is_signal_frame (cursor) > 0)
    return riscv_handle_signal_frame (cursor);

  /* Restore default memory validation state */
  c->validate = validate;

  /* Try DWARF-based unwinding... */
  ret = dwarf_step (&c->dwarf);

  if (unlikely (ret == -UNW_ESTOPUNWIND))
    return ret;

  /* DWARF unwinding didn't work, let's tread carefully here */
  if (unlikely (ret < 0))
  ......

  return (c->dwarf.ip == 0) ? 0 : 1;
}
```

其中会判断是否当前 frame 是 signal frame，如果是则走特殊的针对 signal frame 的处理，否则就是普通函数，走 `dwarf_step` 函数，就是根据 CFI 信息进行处理。

我们来看看判断是否是 signal frame 的逻辑 [`unw_is_signal_frame` 函数][18], 看来这个函数也是专门为 Linux 做的。

```cpp
#ifdef __linux__

/*
  The stub looks like:
  addi x17, zero, 139    0x08b00893
  ecall                  0x00000073
  See <https://github.com/torvalds/linux/blob/44db63d1ad8d71c6932cbe007eb41f31c434d140/arch/riscv/kernel/vdso/rt_sigreturn.S>.
*/
#define SIGRETURN_I0 0x08b00893
#define SIGRETURN_I1 0x00000073

#endif /* __linux__ */

int
unw_is_signal_frame (unw_cursor_t *cursor)
{
#ifdef __linux__
  struct cursor *c = (struct cursor*) cursor;
  unw_word_t i0, i1, ip;
  unw_addr_space_t as;
  unw_accessors_t *a;
  void *arg;
  int ret;

  as = c->dwarf.as;
  a = unw_get_accessors_int (as);
  arg = c->dwarf.as_arg;

  ip = c->dwarf.ip;

  if (!ip || !a->access_mem || (ip & (sizeof(unw_word_t) - 1)))
    return 0;

  if ((ret = (*a->access_mem) (as, ip, &i0, 0, arg)) < 0)
    return ret;

  if ((ret = (*a->access_mem) (as, ip + 4, &i1, 0, arg)) < 0)
    return ret;

  if ((i0 & 0xffffffff) == SIGRETURN_I0 && (i1 & 0xffffffff) == SIGRETURN_I1)
    {
      Debug (8, "cursor at signal frame\n");
      return 1;
    }

  return 0;
#else
  return -UNW_ENOINFO;
#endif
}
```

参考例子代码 [test_libunwind_signal.c][19]，当程序按照如下顺序调用：`signal_handler-> unwind_by_libunwind_nongnu -> unw_step`，此时 cursor 指向 `signal_handler` 函数对应的 stack frame，则 `ip = c->dwarf.ip;` 这条语句实际上拿到的就是 `signal_handler` 函数对应的 stack frame 中保存的 RA，根据我们在 [《聊一聊 Linux 上信号处理过程中的信号栈帧》][6] 中的分析过，内核的骚操作，确保了 signal handler 的 stack frame 中保存的 RA 指向的就是 `__vdso_rt_sigreturn` 这个函数。然后 `unw_is_signal_frame` 函数通过检查 RA 指向内存的内容，如果内存中的内容和 `__vdso_rt_sigreturn` 这个函数的指令相同，说明 cursor 的上一帧就是 signal frame，即 `struct rt_sigframe` 所在的那块内存。实际上我们可以把 signal frame 就认为是 `__vdso_rt_sigreturn` 的栈帧。 

判断出来后，实际的处理在 [`riscv_handle_signal_frame` 函数][20]。这里实际上就是在根据 `struct rt_sigframe` 的信息修改下一次的 cursor。因为我们知道 `struct rt_sigframe` 中保存了用户程序进入内核态之前的现场，也就是 [test_libunwind_signal.c][19] 中 `raise` 函数的现场，这样整个 stack unwinding 就连接起来了。`riscv_handle_signal_frame` 函数的代码列在下面，方便大家参考阅读。

```cpp
static int
riscv_handle_signal_frame (unw_cursor_t *cursor)
{
  int ret, i;
  struct cursor *c = (struct cursor *) cursor;
  unw_word_t sp, sp_addr = c->dwarf.cfa;
  struct dwarf_loc sp_loc = DWARF_LOC (sp_addr, 0);

  if ((ret = dwarf_get (&c->dwarf, sp_loc, &sp)) < 0)
    return -UNW_EUNSPEC;

  if (!unw_is_signal_frame (cursor))
    return -UNW_EUNSPEC;

#ifdef __linux__
  /* rt_sigframe contains the siginfo structure, the ucontext, and then
     the trampoline. We store the mcontext inside ucontext as sigcontext_addr.
  */
  c->sigcontext_format = RISCV_SCF_LINUX_RT_SIGFRAME;
  c->sigcontext_addr = sp_addr + sizeof (siginfo_t) + UC_MCONTEXT_REGS_OFF;
  c->sigcontext_sp = sp_addr;
  c->sigcontext_pc = c->dwarf.ip;
#else
  /* Not making any assumption at all - You need to implement this */
  return -UNW_EUNSPEC;
#endif

  /* Update the dwarf cursor.
     Set the location of the registers to the corresponding addresses of the
     uc_mcontext / sigcontext structure contents.  */

#define  SC_REG_OFFSET(X)   (8 * X)

  /* The PC is stored in place of X0 in sigcontext */
  c->dwarf.loc[UNW_TDEP_IP] = DWARF_LOC (c->sigcontext_addr + SC_REG_OFFSET(UNW_RISCV_X0), 0);

  for (i = UNW_RISCV_X1; i <= UNW_RISCV_F31; i++)
    {
      c->dwarf.loc[i] = DWARF_LOC (c->sigcontext_addr + SC_REG_OFFSET(i), 0);
    }

  /* Set SP/CFA and PC/IP.  */
  dwarf_get (&c->dwarf, c->dwarf.loc[UNW_TDEP_SP], &c->dwarf.cfa);
  dwarf_get (&c->dwarf, c->dwarf.loc[UNW_TDEP_IP], &c->dwarf.ip);

  return 1;
}
```




[1]: https://www.nongnu.org/libunwind/
[2]: https://github.com/llvm/llvm-project/blob/main/libunwind/docs/index.rst
[3]: https://clang.llvm.org/docs/Toolchain.html#unwind-library
[4]: ./20220717-call-stack.md
[5]: ./20220719-stack-unwinding.md
[6]: ./20220816-signal-frame.md
[7]: https://gcc.gnu.org/onlinedocs/gccint/Exception-handling-routines.html#Exception-handling-routines
[8]: https://refspecs.linuxbase.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/libgcc-s.html
[9]: https://gcc.gnu.org/onlinedocs/gcc/Return-Address.html#Return-Address
[10]: https://github.com/pathscale/libunwind
[11]: ./code/20220819-libunwind/test_builtin.c
[12]: ./code/20220721-stackuw-cfi/backtrace.c
[13]: https://github.com/libunwind/libunwind/blob/master/include/unwind.h
[14]: https://itanium-cxx-abi.github.io/cxx-abi/abi-eh.html#base-abi
[15]: ./code/20220819-libunwind/libunwind.c
[16]: ./code/20220819-libunwind/test_libunwind.c
[17]: https://github.com/libunwind/libunwind/blob/v1.6.2/src/riscv/Gstep.c#L78
[18]: https://github.com/libunwind/libunwind/blob/v1.6.2/src/riscv/Gis_signal_frame.c#L44
[19]: ./code/20220819-libunwind/test_libunwind_signal.c
[20]: https://github.com/libunwind/libunwind/blob/v1.6.2/src/riscv/Gstep.c#L30
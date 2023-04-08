![](./diagrams/linker-loader.png)

文章标题：**RISC-V 中的 global pointer 寄存器**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [参考](#参考)
- [分析](#分析)

<!-- /TOC -->

# 参考

- [Ref 1] [The RISC-V Instruction Set Manual Volume I: Unprivileged ISA Document Version 20191213][1]
- [Ref 2] [RISC-V ABIs Specification Version 1.0: Ratified][2]
- [Ref 3] [gcc gp (global pointer) register][8]
- [Ref 4] [[llvm-dev] Default linker script used by lld][9]
- [Ref 5] [All Aboard, Part 3: Linker Relaxation in the RISC-V Toolchain][10]


# 分析

在 [Ref 1] 的中，RISC-V 给 x3 这个通用寄存器起了一个 ABI 别名为 gp（Global pointer）。以前一直没太深入考虑过 gp 的含义和用法。最近研究了一下，差不多清楚了，记录下来备忘。 

![][3]

具体参考 [Ref 2] 的 “9.1.4. Global-pointer Relaxation” 章节，大致的意思是说 RISC-V ISA 预留了 x3 这个寄存器主要是用来做 Linker Relaxtion 的。

> **Description**
>
>   This relaxation type can relax a sequence of the load address of a symbol or load/store with a
>   symbol reference into global-pointer-relative instruction.

也就是说通过利用 gp 来帮助 linker 对一些基于符号（symbol）加载地址（load address）和访问内存（load/store）的指令序列进行优化，可以压缩指令的条数。

举个例子理解一下，基于 musl + riscv gnu tools，有关 musl 的构建可以参考 [《musl 构建说明》][4]：

```cpp
$ cat test.c
#include <stdio.h>

int myvar;

int main()
{
    return myvar;
}
```

先编译一下 test.c，带上 -g 为了方便反汇编查看生成的 obj 文件中的指令：
```shell
$ musl-gcc -c -g test.c
$ riscv64-unknown-linux-gnu-objdump -dSr test.o
......
    return myvar;
   6:	000007b7          	lui	a5,0x0
			6: R_RISCV_HI20	myvar
			6: R_RISCV_RELAX	*ABS*
   a:	0007a783          	lw	a5,0(a5) # 0 <main>
			a: R_RISCV_LO12_I	myvar
			a: R_RISCV_RELAX	*ABS*
}
   e:	853e                	mv	a0,a5
......
```

可见编译器对通过 myvar 加载内存数据的指令除了给出 `R_RISCV_HI20` 和 `R_RISCV_LO12_I` 这些 Target Relocation 外还加上了 `R_RISCV_RELAX` 的 relocation 指示，在链接阶段 Linker 会根据这个指示，针对 `R_RISCV_HI20` 和 `R_RISCV_LO12_I` 所对应的指令进行 relaxation 优化。

然后再完整生成 `a.out`，再看看 `return myvar;` 对应的指令。

```shell
$ musl-gcc -g test.c
$ riscv64-unknown-linux-gnu-objdump -dSr a.out
......
    return myvar;
   10536:	8581a783          	lw	a5,-1960(gp) # 12058 <myvar>
}
   1053a:	853e                	mv	a0,a5
......
```

继续借用 [Ref 1] 上对 Relaxation 的定义描述一下操作的效果：

> **Relaxation**
>
> • Instruction associated with `R_RISCV_HI20` or `R_RISCV_PCREL_HI20` can be removed.
>
> • Instruction associated with `R_RISCV_LO12_I`, `R_RISCV_LO12_S`, `R_RISCV_PCREL_LO12_I` or
>   `R_RISCV_PCREL_LO12_S` can be replaced with a global-pointer-relative access instruction.

大致的意思就是，以这里的例子为例，Relaxation 的效果就是在链接过程中，Linker 对原来需要用两条指令完成的加载内存数据的操作（lui + lw），可以压缩为一条 lw（该 lw 的地址访问是以 gp 为基址进行偏移一个立即数）。

而 gp 就是我们前面说的 x3，很显然，这个 gp 需要在某个地方初始化为一个值。那么这个初始化又该由谁完成，以及初始化为什么值呢？继续阅读 [Ref 1] 给了我们回答：

> The global-pointer refers to the address of the `__global_pointer$` symbol, which is
> the content of gp register.
>
> This relaxation requires the program to initialize the gp register with the address
> of `__global_pointer$` symbol before accessing any symbol address, strongly
> recommended initialize gp at the beginning of the program entry function like
> `_start`, and code fragments of initialization must disable linker relaxation to
> prevent initialization instruction relaxed into a NOP-like instruction (e.g. mv gp,
> gp).

啰里啰唆讲了一大堆，虽然看上去是要我们写程序（program）时自己去初始化这个 gp 寄存器，但实际上这个动作都是由 c 库帮助我们去完成的，ISA 规范中也提了最好是在 `_start` 中做，就是比较晦涩没有明说是 c 库，可是舍我其谁呢。

参考一下 musl 的代码，看一下 riscv64 的 [`crt_arch.h`][15]:

```cpp
__asm__(
".section .sdata,\"aw\"\n"
".text\n"
".global " START "\n"
".type " START ",%function\n"
START ":\n"
".weak __global_pointer$\n"
".hidden __global_pointer$\n"
".option push\n"
".option norelax\n\t"
"lla gp, __global_pointer$\n"
".option pop\n\t"
......
```

可以看到 `lla gp, __global_pointer$` 就是在对 gp 进行赋值，注意这里使用了 lla 这个 pseudoinstruction。参考 [Ref 1] 的 Table 25.2 如下：

![][6]

本质上是一个相对于当前 PC 访问符号地址的操作，和 [Ref 2] 中示例代码中的如下指令序列是一回事。
```cpp
auipc gp, %pcrel_hi(__global_pointer$)
addi gp, gp, %pcrel_lo(1b)
```

另外注意在初始化 gp 指令的操作中必须要通知 linker 不要做 relaxation，避免 linker 将这两条指令优化成 （`mv
 gp, gp`）。所以我们会看到 musl 代码中给 lla 指令加上了 `.option norelax` 的 directive。

还剩下最后一个问题就是 `__global_pointer$` 这个符号定义在什么地方？其值怎么决定？

既然具体的 relaxation 是由 Linker 完成，所以这个 `__global_pointer$` 符号的值自然也是由 Linker 来决定。如果不指定的话，Linker 在默认的 Linker Script 中会定义这个符号。

以 ld 为例，我们可以通过如下命令查看其默认的 Linker Script 的内容：
```shell
$ riscv64-unknown-linux-gnu-ld -verbose
......
  __BSS_END__ = .;
    __global_pointer$ = MIN(__SDATA_BEGIN__ + 0x800,
		            MAX(__DATA_BEGIN__ + 0x800, __BSS_END__ - 0x800));
......
```

LLVM 的 lld 和 GNU ld 不同，不支持导出默认的 Linker Script 的内容，所以我们不得不在代码中查看其定义，具体见 [`lld/ELF/Writer.cpp`][7]:

```cpp
    // RISC-V's gp can address +/- 2 KiB, set it to .sdata + 0x800. This symbol
    // should only be defined in an executable. If .sdata does not exist, its
    // value/section does not matter but it has to be relative, so set its
    // st_shndx arbitrarily to 1 (Out::elfHeader).
    if (config->emachine == EM_RISCV && !config->shared) {
      OutputSection *sec = findSection(".sdata");
      addOptionalRegular("__global_pointer$", sec ? sec : Out::elfHeader, 0x800,
                         STV_DEFAULT);
    }
```








[1]:https://github.com/riscv/riscv-isa-manual/releases/download/Ratified-IMAFDQC/riscv-spec-20191213.pdf
[2]:https://github.com/riscv-non-isa/riscv-elf-psabi-doc/releases/download/v1.0/riscv-abi.pdf
[3]:./diagrams/20230408-global-pointer/gp.png
[4]:./20230401-musl-build.md
[5]:https://git.musl-libc.org/cgit/musl/tree/arch/riscv64/crt_arch.h?h=v1.2.3
[6]:./diagrams/20230408-global-pointer/lla.png
[7]:https://github.com/llvm/llvm-project/blob/llvmorg-16.0.1/lld/ELF/Writer.cpp#L1855
[8]:https://groups.google.com/a/groups.riscv.org/g/sw-dev/c/60IdaZj27dY/m/TKT3hbNlAgAJ
[9]:https://groups.google.com/g/llvm-dev/c/3y15MZRgVZ4
[10]:https://www.sifive.com/blog/all-aboard-part-3-linker-relaxation-in-riscv-toolchain
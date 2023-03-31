![](./diagrams/linker-loader.png)

文章标题：**musl 的 build system 分析**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. configure](#2-configure)
	- [2.1. configure 脚本分析](#21-configure-脚本分析)
- [3. Makefile](#3-makefile)

<!-- /TOC -->

具体构建 musl 的总结参考另一篇笔记 [《musl 构建说明》][1]。本文则是从 musl 的源码角度分析一下其 build system 做了些什么。

从 [《musl 构建说明》][1] 中我们知道整个 musl 的构建主要就是分两步，第一步通过运行 `configure`，在 build 目录下生成一个 `config.mak`，这个 `config.mak` 文件中定义了第二步 make 需要的各种变量，这些变量实现了针对不同的 ARCH，不同的构建需求，影响着第二步 make 所使用的 Makefile 的内容。所以本文就针对 configure 和 Makefile 简单总结分析一下 musl 的 build system。


# 1. 参考文档

- musl 源码 v1.2.3

# 2. configure

musl 源码根目录下的 `configure` 文件实际上就是一个 shell 脚本。可以接受参数控制，具体使用可以带上 `-h` 选项运行 `configure` 查看：

```shell
configure -h
```

`configure` 的功能主要是分析给定的参数选项，并最终在 build 目录下生成一个 `config.mak` 和 一个 `Makefile`, 而 `Makefile` 实际上是指向 musl 源码目录下的 `Makefile`，所以主要是生成了一个 `config.mak`。

```shell
ls -l
total 4
-rw-rw-r-- 1 wangchen wangchen 1808 Mar 17 17:20 config.mak
lrwxrwxrwx 1 wangchen wangchen   16 Mar 17 17:20 Makefile -> ../musl/Makefile
```
而这个 `config.mak` 文件中保存的信息就是一些全局定义的环境变量，为下一步运行 make 时被源码目录下的 `Makefile` 所使用。

```shell
cat config.mak 
......
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
ARCH = x86_64
......
```

## 2.1. configure 脚本分析

直接看一下 `configure` 文件的内容，文件不长，具体注释就嵌入在代码中了，为了和原有的注释区别，我加的注释用 `###` 开头

```bash
#!/bin/sh

### usage() 函数，执行 `configure -h` 时被调用
usage () {
......
}

### 一些辅助函数：
### - quote: 
### - ...
### - fnmatch: 检查字符串中是否部分匹配
### - cmdexists: 检查一个 command 是否执行成功，对应的 ### bin 是否存在
### - trycc： 如果 CC 没有定义，则检查 CROSS_COMPILE 编译器是否存在。并设置 CC 环境变量
# Helper functions
......

# Beginning of actual script

### 定义脚本中会用到的变量，其中部分变量也会作为 config 变量输出到 config.mak 中
CFLAGS_C99FSE=
......

### 下面开始 configure 的主体流程，整个过程分为几个主要步骤：
### - 选项解析
### - 检查编译器，这部分是处理的大头
### - 结果输出到 config.mak
### - 创建 Makefile 符号链接

### Step1: 选项解析，即根据命令行参数对变量赋值，同时会做相应的检查
### 注意：命令行选项 `--with-malloc` 用于选择我们使用哪种 malloc 的实现方法，对应源码中的
### - src/malloc/mallocng
### - src/malloc/oldmalloc
### 默认是 mallocng，即 next generation，是新的实现
for arg ; do
......

### Step2: 编译器检查

### 通过检查和 CROSS COMPILE 相关的几个关键参数推导 CROSS_COMPILE
### --target=TARGET         configure to run on target TARGET [detected]
### --host=HOST             same as --target
### --build=BUILD           build system type; used only to infer cross-compiling
### 所以我们在交叉编译 riscv 的时候需要指定 `CROSS_COMPILE=riscv64-unknown-linux-gnu-`，
### 这样 build system 就会直接用我们给的，不会自己定义默认的
#
# Check whether we are cross-compiling, and set a default
# CROSS_COMPILE prefix if none was provided.
#
test "$target" && \
test "$target" != "$build" && \
test -z "$CROSS_COMPILE" && \
CROSS_COMPILE="$target-"

### 检查工具链是否存在，从脚本上看，会优先 CC
### 如果 CC 定义了，则不考虑 CROSS_COMPILE 前缀。所以如果我们要用 clang，只要设置 CC=clang
#
# Find a C compiler to use
#
printf "checking for C compiler... "
trycc ${CROSS_COMPILE}gcc
trycc ${CROSS_COMPILE}c99
trycc ${CROSS_COMPILE}cc
printf "%s\n" "$CC"
test -n "$CC" || { echo "$0: cannot find a C compiler" ; exit 1 ; }

### 检查使用该编译器，以及 $CPPFLAGS $CFLAGS 编译（但不链接）一个 c 源文件是否会成功
printf "checking whether C compiler works... "
echo "typedef int x;" > "$tmpc"
if output=$($CC $CPPFLAGS $CFLAGS -c -o /dev/null "$tmpc" 2>&1) ; then
printf "yes\n"
else
printf "no; compiler output follows:\n%s\n" "$output"
exit 1
fi

### 进一步检查编译器选项，包括预处理选项 trycppif、编译选项 tryflag 和链接选项 tryldflag
### 以 tryflag 为例
### 利用 tryflag 定义各个 CFLAGS_C99FSE/CFLAGS_NOSSP/CFLAGS_MEMOPS
### 以 `tryflag CFLAGS_C99FSE -std=c99` 为例，检测 -std=c99 是否对当前 compiler 支持，
### 如果支持就添加到 CFLAGS_C99FSE 环境变量中
### 在检查过程中会打印 "checking whether compiler accepts ..."，所以我们在 
### configure 的 log 中会看到：
### checking whether compiler accepts -std=c99... yes
### checking whether compiler accepts -nostdinc... yes
### ......
### 最后在 `config.mak` 文件中会得到合法的 CFLAGS_C99FSE 值，譬如：
### CFLAGS_C99FSE = -std=c99 -nostdinc -ffreestanding ......
......

### Step3: 结果输出到 config.mak
printf "creating config.mak... "
......

### Step4: 创建 Makefile 符号链接
test "$srcdir" = "." || ln -sf $srcdir/Makefile .

printf "done\n"
```

# 3. Makefile

musl 整个项目只有一个 Makefile，足够的简单，所以还能在这里掰扯一下。

首先定义了一堆的 Make 变量，然后 include 两份 `.mak` 文件，一份是 configure 生成的 `config.mak`、另一份是 ARCH 相关的 `arch.mak`。之所以在定义变量后再 include，意图很明显，就是利用后 include 的 `.mak` 文件中定义的变量来 override 先定义的变量，`config.mk` 给了我们通过执行 `configure` 命令对 build system 定制的途径，而 ARCH 相关的 `arch.mak` 则是针对一些 ARCH 的变化我们可以进一步在代码中进行定制。

```makefile
-include config.mak
-include $(srcdir)/arch/$(ARCH)/arch.mak
```

貌似 musl 的 build system 提供了 SUBARCH 的机制，但是具体怎么用， 还要再看看。
这也是 https://ithelp.ithome.com.tw/articles/10262992 中提出简单起见，先从 riscv64 赋值一份平级的 riscv32 进行移植的原因

Makefile 的其余大部分都是在描述 rules

这些规则的目标主要包括：

- `all`: 当我们输入 make 时默认执行 `make all`，all 是 Makefile 中定义的第一个 target，也就是我们的终极目标。

  ```makefile
  all: $(ALL_LIBS) $(ALL_TOOLS)
  ```

  最终生成的结果 (ALL_LIBS) 包含如下几个重要的子项（这里先罗列一下，具体这些生成的文件的用处，后面再分析）：
  - `CRT_LIBS`: `build/lib/*.o`, 包括：`crt1.o`, `crti.o`, `crtn.o`, `rcrt1.o`, `Scrt1.o`
  - `STATIC_LIBS`: `build/lib/libc.a`
  - `SHARED_LIBS`: `build/lib/libc.so`
  - `EMPTY_LIBS`: `build/lib/libXXX.a`, XXX 包括：`{m rt pthread crypt util xnet resolv dl}`, 这些文件很特殊，每个文件只有 8 个字节大小，内容一样，用 cat 查看都是 "!<arch>"， 用 hexdump 看是 `3c21 7261 6863 0a3e`
  - `TOOL_LIBS`: 即 `build/lib/musl-gcc.specs`

  分析 make 的执行过程打印可以看到上面这些子项的生成过程。

- `install`

  注意看 Makefile 中的如下规则：

  ```makefile
  install-libs: $(ALL_LIBS:lib/%=$(DESTDIR)$(libdir)/%) $(if $(SHARED_LIBS),$(DESTDIR)$(LDSO_PATHNAME),)

  install-headers: $(ALL_INCLUDES:include/%=$(DESTDIR)$(includedir)/%)

  install-tools: $(ALL_TOOLS:obj/%=$(DESTDIR)$(bindir)/%)

  install: install-libs install-headers install-tools
  ```

[1]:./20230401-musl-build.md
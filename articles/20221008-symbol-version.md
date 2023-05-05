![](./diagrams/linker-loader.png)

文章标题：**学习笔记: Symbol Versioning 基本使用**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

本文主要记录了一些学习 Symbol Versioning 中的理解，并自己写了一些例子对概念进行了验证。

文章大纲

<!-- TOC -->

- [1. 参考：](#1-参考)
- [2. 问题的产生](#2-问题的产生)
- [3. Symbol Versioning 机制](#3-symbol-versioning-机制)
- [4. GNU 对 Symbol Versioning 的扩展](#4-gnu-对-symbol-versioning-的扩展)

<!-- /TOC -->

# 1. 参考：

- 【参考 1】《程序员的自我修养-链接、装载与库》8.2 符号版本
- 【参考 2】[GNU ld 手册 3.9 VERSION Command](https://sourceware.org/binutils/docs/ld/VERSION.html)
- 【参考 3】[Symbol Versioning](https://man7.org/conf/lca2006/shared_libraries/slide19a.html)
- 【参考 4】[How the GNU C Library handles backward compatibility](https://developers.redhat.com/blog/2019/08/01/how-the-gnu-c-library-handles-backward-compatibility#)
- 【参考 5】[What is symbol versioning good for? Do I need it?](https://sourceware.org/glibc/wiki/FAQ#What_is_symbol_versioning_good_for.3F__Do_I_need_it.3F)

# 2. 问题的产生

【参考 1】的 8.2 章节从历史的角度回顾了这个问题的产生。简单总结如下：

早期的系统，static linker 在链接应用程序和 so 时，会将程序所依赖的 so 的名字、主版本号（major version）和次版本号（minor version）都记录到应用程序 `a.out` 中，假设 so 的信息为 `libfoo.so.X.Y`，其中 `X` 为 major version，`Y` 为 minor version。执行 `a.out` 时 dynamic linker 根据应用程序自身记录的依赖的 so 的版本信息和运行系统中的实际 so 的版本信息进行对比即可知道是否匹配。假设运行系统中 so 的版本信息为 `libfoo.so.X'.Y'`。X 和 X' 肯定要相同，如果不同，说明 ABI 有重大更新，那是完全不兼容的情况，这里暂时不考虑。如果 `Y' >= Y`，说明运行系统上的 so 的版本比 `a.out` 链接时依赖的 so 的版本新，那么根据 minor version 支持 backward compatible 的特性，dynamic linker 就知道运行系统上的 so 满足运行要求。

但考虑到对于 `Y' < Y` 的场景，即当前运行系统中存在的 so 的版本比 `a.out` 链接指定的 so 版本要旧，则会发生一种两难的情况。具体解释为，当 `Y' < Y` 时，早期的 dynamic linker 采取的策略无非有两种，一种是简单告警，程序继续运行，如果 `a.out` 用到的接口都比较老，即低于 `Y'` 版本，那么很好，一切正常；如果很不幸 `a.out` 的确用到了 `Y` 版本新加的接口，那么运行过程中会发生重定位失败导致程序异常，这种处理方式比较宽松，缺点是可能会存在一开始掩盖错误的发生，而在你不注意的情况下当执行到新接口调用时发生暴雷。所以也有人采用了第二种简单粗暴的策略就，只要发现 `Y' < Y`，dynamic linker 就报错，阻止 `a.out` 执行，但这么做的缺点也同样存在，就是可能 `a.out` 根本没有用到 `Y` 新增的接口，而这种情况在第一种策略时程序还是有机会得到执行的。这个问题 【参考 1】 上称之为 **“版本交会问题（Minor-revison Rendezvous Problem）”**，其实就是我们这里说的 `Y' < Y`。

【参考 1】的 8.2 章节也提到了，在引入 SONAME 机制后，`Y' < Y` 的问题依然无法圆满解决。其实在引入 SONAME 后 dynamic linker 只检查了 major version，连 minor version 都不检查了，而且 SONAME 只考虑了系统上 so 升级过程的问题，对于解决 `Y' < Y` 问题我理解其实毫无用处。

# 3. Symbol Versioning 机制

由【参考 1】可知为了解决 **“版本交会问题（Minor-revison Rendezvous Problem）”**，Linux 上的 Glibc 从版本 2.1 开始引入 Symbol Versioning 的机制。虽然这个机制主要被 libc 库所使用，实际上任何基于 ELF 的 so 都可以利用改机制对导出的符号进行版本管理。

我目前理解 Symbol Versioning 之所以能够解决 **“版本交会问题”**，根本原因是 Symbol Versioning 在更细的颗粒度上支持了版本化。原来的共享库版本机制和 SONAME 机制是在 so 的模块级别实现了版本化，而 Symbol Versioning 则针对每个 so 模块文件中的每个符号都支持了版本化。

具体的 Symbol Versioning 的使用语法我不想再这里再写一遍，本身相对简单，可以参考官方手册的 【参考 2】了解。本文只想通过实际例子来总结一下以备忘。

针对语法部分，这里简单补充一些自己的理解：

- 如果一个符号没有通过 version script 指定版本信息，则在 so 中的符号也没有符号信息，可执行程序中记录的未决符号也不含版本信息，针对这种情况 dynamic linker 也不会对符号的版本进行检查。具体参考 [例子中的 `nover_test`][1]。
- 如果一个符号出现在 version node 中但没有说明是 global 还是 local，默认为 global
- 所谓的继承，可以认为是超集。
- 同一个符号不允许重复绑定同一个 version node
- 不允许同一个符号绑定多个 version node 并赋予不同的 global/local 属性

下面这个例子主要是验证一下引入 Symbol Versioning 机制后如何解决 **“版本交会问题”**。

`libfoo.so.1.1`、`libfoo.so.1.2` 和 `libfoo.so.1.3` 是我们制作的 libfoo 的三个版本，1.1 中只有一个函数 `foo1`，1.2 中在 1.1 的基础上新增一个函数 `foo2`，1.3 在 1.2 的基础上新增一个函数 `foo3`。制作这三个库的 version script 文件分别是 `foo_1_1.map`、`foo_1_2.map` 和 `foo_1_3.map`。以 `foo_1_3.map` 为例，其他具体的代码参考 [这里][1]：

```bash
$ cat foo_1_3.map
VER_1.1 {
        global: foo1;
};

VER_1.2 {
        global: foo2;
} VER_1.1;

VER_1.3 {
        global: foo3;
} VER_1.2;
```
执行如下命令生成 libfoo 并安装，以 `libfoo.so.1.3` 为例：

```bash
$ gcc  -c -fPIC -Wall foo_1_3.c
$ gcc  -shared -o libfoo.so.1.3 foo_1_3.o -Wl,--version-script=foo_1_3.map -Wl,-soname,libfoo.so.1 
$ rm -f libfoo.so libfoo.so.1
$ ln -s libfoo.so.1.3 libfoo.so
$ ln -s libfoo.so.1.3 libfoo.so.1
$ ls -l
lrwxrwxrwx 1 wangchen wangchen    13 Oct  8 20:27 libfoo.so -> libfoo.so.1.3
lrwxrwxrwx 1 wangchen wangchen    13 Oct  8 20:27 libfoo.so.1 -> libfoo.so.1.3
-rwxrwxr-x 1 wangchen wangchen 16456 Oct  8 20:27 libfoo.so.1.3
```
顺便提一下，在安装 so 文件时，我们一般会附加安装两个符号链接，以 libfoo 为例：

- `libfoo.so.1.2`：真正的 so 文件本体，这个文件名我们也称之为 real-name。
- `libfoo.so`：指向 real-name 的符号链接，称之为 linker-name，用于静态链接时的 `-lfoo`
- `libfoo.so.1`：同样是指向 real-name 的符号链接，称为 soname，也是 `foo_test` 中保存的依赖的 so 文件的名字（`ldd foo_test` 会列出所有的依赖）。执行 `foo_test` 时 dynamic linker 会根据这个名字去找实际的 so 文件

查看这些 libfoo 的 dynamic symbol table，可以看到其导出的函数符号都是带上版本控制信息了。以 `libfoo.so.1.3` 为例：
```bash
$ readelf --dyn-syms -W ./libfoo.so.1.3

Symbol table '.dynsym' contains 12 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     ......
     6: 0000000000001119    23 FUNC    GLOBAL DEFAULT   15 foo1@@VER_1.1
     ......
     8: 0000000000001130    23 FUNC    GLOBAL DEFAULT   15 foo2@@VER_1.2
     ......
    11: 0000000000001147    23 FUNC    GLOBAL DEFAULT   15 foo3@@VER_1.3
```

现在我们来看一下此时 **“版本交会问题”** 是否可以解决。

现在我们基于最新的 `libfoo.so.1.3` 链接生成 foo_test, 这里 `Y` 的值就是 3：

```bash
$ gcc foo_test.c -Wl,-rpath,`pwd` -o foo_test -lfoo -L.
```

注意这个程序中只利用了 1.2 之前的函数接口 `foo1` 和 `foo2`。所以如果我们同样查看 `foo_test` 的 dynamic symbol table，我们会发现 `foo_test` 上记住了它依赖的两个外部符号 `foo1` 和 `foo2` 以及它们的版本号。

```bash
$ readelf --dyn-syms -W ./foo_test

Symbol table '.dynsym' contains 8 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     ......
     2: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND foo2@VER_1.2 (2)
     ......
     6: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND foo1@VER_1.1 (4)
     7: 0000000000000000     0 FUNC    WEAK   DEFAULT  UND __cxa_finalize@GLIBC_2.2.5 (3)
```

如果将当前运行环境中的 libfoo 换成 `libfoo.so.1.2` 版本的，如下所示，也就是说运行环境中 `Y'` 的值这里为 2，所以 `Y' < Y`。

```bash
$ gcc  -c -fPIC -Wall foo_1_2.c
$ gcc  -shared -o libfoo.so.1.2 foo_1_2.o -Wl,--version-script=foo_1_2.map -Wl,-soname,libfoo.so.1 
$ rm -f libfoo.so libfoo.so.1
$ ln -s libfoo.so.1.2 libfoo.so
$ ln -s libfoo.so.1.2 libfoo.so.1
$ ls -l
......
-rwxrwxr-x 1 wangchen wangchen 16736 Oct  8 17:16 foo_test
lrwxrwxrwx 1 wangchen wangchen    13 Oct  8 17:27 libfoo.so -> libfoo.so.1.2
lrwxrwxrwx 1 wangchen wangchen    13 Oct  8 17:27 libfoo.so.1 -> libfoo.so.1.2
-rwxrwxr-x 1 wangchen wangchen 16392 Oct  8 17:27 libfoo.so.1.2
```

此时运行 `foo_test`，一切正常，因为 dynamic linker 在检查 `foo_test` 中的未决符号 `foo1` 和 `foo2` 的版本时和当前 `libfoo.so.1.2` 中的符号版本是匹配的。

```bash
$ ./foo_test
foo1
foo2
$ readelf --dyn-syms -W ./libfoo.so.1.2

Symbol table '.dynsym' contains 10 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     ......
     6: 0000000000001119    23 FUNC    GLOBAL DEFAULT   15 foo1@@VER_1.1
     7: 0000000000001130    23 FUNC    GLOBAL DEFAULT   15 foo2@@VER_1.2
```

如果把当前运行环境中的 libfoo 换成 `libfoo.so.1.1` 版本的，执行 `foo_test` 就会报错，dynamic linker 会发现 `foo_test` 中有个未决的符号 `foo2@VER_1.2` 在 `libfoo.so.1.1` 中不存在。

```bash
$ gcc  -c -fPIC -Wall foo_1_1.c
$ gcc  -shared -o libfoo.so.1.1 foo_1_1.o -Wl,--version-script=foo_1_1.map -Wl,-soname,libfoo.so.1 
$ rm -f libfoo.so libfoo.so.1
$ ln -s libfoo.so.1.1 libfoo.so
$ ln -s libfoo.so.1.1 libfoo.so.1
$ ls -l
......
-rwxrwxr-x 1 wangchen wangchen 16736 Oct  8 20:54 foo_test
lrwxrwxrwx 1 wangchen wangchen    13 Oct  8 21:01 libfoo.so -> libfoo.so.1.1
lrwxrwxrwx 1 wangchen wangchen    13 Oct  8 21:01 libfoo.so.1 -> libfoo.so.1.1
-rwxrwxr-x 1 wangchen wangchen 16336 Oct  8 21:01 libfoo.so.1.1
$ readelf --dyn-syms -W ./libfoo.so.1.1

Symbol table '.dynsym' contains 8 entries:
     ......
     6: 0000000000001119    23 FUNC    GLOBAL DEFAULT   15 foo1@@VER_1.1
     7: 0000000000000000     0 OBJECT  GLOBAL DEFAULT  ABS VER_1.1
$ ./foo_test 
./foo_test: /aosp/wangchen/dev-aosp12/working-group/articles/code/20221008-symbol-version/libfoo.so.1: version `VER_1.2' not found (required by ./foo_test)
```

# 4. GNU 对 Symbol Versioning 的扩展

引入这个扩展功能的目的是为了解决如下问题，引用 【参考 5】 的话：

> Symbol versioning solves problems that are related to interface changes. One version of an interface might have been introduced in a previous version of the GNU C library but the interface or the semantics of the function has been changed in the meantime. For binary compatibility with the old library, a newer library needs to still have the old interface for old programs. On the other hand, new programs should use the new interface. Symbol versioning is the solution for this problem. The GNU libc uses symbol versioning by default unless it gets disabled via a configure switch.

下面通过一个例子来理解，具体代码参考 [例子][1]。这里的例子体现的是接口 foo 没有变，但是改变了 foo 的语义和功能，所以是一种针对不兼容升级下的处理场景。

假设我们先做了 libsv 的 1.1 版本：

```bash
$ gcc  -c -fPIC -Wall sv_v1.c
$ gcc  -shared -o libsv.so.1.1 sv_v1.o -Wl,--version-script=sv_v1.map -Wl,-soname,libsv.so.1 
$ rm -f libsv.so libsv.so.1
$ ln -s libsv.so.1.1 libsv.so
$ ln -s libsv.so.1.1 libsv.so.1
```

然后基于 1.1 版本的 libsv 制作了应用 `sv_v1_test`，程序代码调用 `foo` 函数
```bash
$ gcc sv_test.c  -Wl,-rpath,`pwd` -o sv_v1_test -lsv -L.
```

假设当前运行环境安装的是 `libsv.so.1.1`，执行结果如下:
```bash
$ ./sv_v1_test 
v1 foo
```

假设我们需要升级 libsv 的下一个版本，但这里我们改变了 foo 函数的实现，输出 "v2 foo"，但是我们没有升级应用，我们还需要 `sv_v1_test` 输出 "v1 foo"。这里就要使用 GNU 对 Version Versioning 的扩展功能了。直接看 libsv 的 1.2 版本实现：

```bash
$ cat sv_v2.c
#include <stdio.h>

__asm__(".symver foo_old,foo@VER_1");
__asm__(".symver foo_new,foo@@VER_2");

void foo_old(void)
{
        printf("v1 foo\n");
}

void foo_new(void)
{
        printf("v2 foo\n");
}
```

`.symver` 是汇编的 directive，作用就是将 `foo@VER_1` 和 `foo@@VER_2` 这种带版本的符号绑定到不同的函数实现上。

- `.symver foo_old,foo@VER_1` 表示当应用程序的未决 `foo` 符号是 `VER_1` 时，`foo` 符号对应的是 `foo_old` 这个函数，这里的实现和 v1 中的 `foo` 函数实现相同，达到兼容的目的。
- `.symver foo_new,foo@@VER_2` 表示当应用程序的未决 `foo` 符号是 `VER_2` 时，`foo` 符号对应的是 `foo_new` 这个函数，这么做的效果就是新的和 v2 版本的 so 静态链接的程序将启用新版本的 `foo` 函数。

除了在代码上要添加以上 `.symver` 语句外，同样要提供 version script 文件如下：
```bash
$ cat ./sv_v2.map 
VER_1 {
        global: foo;
        local:  *;      # Hide all other symbols
};

VER_2 {
        global: foo;
        local:  *;      # Hide all other symbols
};
```

我们来验证一下以上效果。首先制作 `libsv.so.1.2` 并替换当前运行环境中的 libsv（升级库）但不改变 `sv_v1_test`

```bash
$ ls -l
lrwxrwxrwx 1 wangchen wangchen    12 Oct  9 09:58 libsv.so -> libsv.so.1.2
lrwxrwxrwx 1 wangchen wangchen    12 Oct  9 09:58 libsv.so.1 -> libsv.so.1.2
-rwxrwxr-x 1 wangchen wangchen 16456 Oct  9 09:58 libsv.so.1.2
-rwxrwxr-x 1 wangchen wangchen 16688 Oct  9 09:45 sv_v1_test
$ ./sv_v1_test 
v1 foo
```
可见这里打印 "v1 foo"，说明 `libsv.so.1.2` 中的 `foo_old` 被调用了。

然后将 `sv_v1_test` 升级替换为 `sv_v2_test`，`sv_v2_test` 程序的代码并无变化，还是用的 `sv_test.c`，但链接时指定了 `libsv.so.1.2`
```bash
$ ls -l
lrwxrwxrwx 1 wangchen wangchen    12 Oct  9 09:58 libsv.so -> libsv.so.1.2
lrwxrwxrwx 1 wangchen wangchen    12 Oct  9 09:58 libsv.so.1 -> libsv.so.1.2
-rwxrwxr-x 1 wangchen wangchen 16456 Oct  9 09:58 libsv.so.1.2
$ gcc sv_test.c  -Wl,-rpath,`pwd` -o sv_v2_test -lsv -L.
$ ./sv_v2_test 
v2 foo
```
说明此时 `libsv.so.1.2` 中的 `foo_new` 被调用了。

但注意如果反向降级 libsv 为 v1 版本，此时执行 `sv_v2_test` 是不可以的，因为查看 `sv_v2_test` 中的 `foo` 已经绑定在 `VER_2`，而我们知道 `libsv.so.1.1` 中提供的 `foo` 符号是绑定在 `VER_1` 上的。

```bash
$ ls -l
lrwxrwxrwx 1 wangchen wangchen    12 Oct  9 10:05 libsv.so -> libsv.so.1.1
lrwxrwxrwx 1 wangchen wangchen    12 Oct  9 10:05 libsv.so.1 -> libsv.so.1.1
-rwxrwxr-x 1 wangchen wangchen 16328 Oct  9 10:05 libsv.so.1.1
-rwxrwxr-x 1 wangchen wangchen 16688 Oct  9 10:02 sv_v2_test
$ ./sv_v2_test
./sv_v2_test: /aosp/wangchen/dev-aosp12/working-group/articles/code/20221008-symbol-version/libsv.so.1: version `VER_2' not found (required by ./sv_v2_test)
```
```
$ readelf --dyn-syms -W ./sv_v2_test

Symbol table '.dynsym' contains 7 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     ......
     2: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND foo@VER_2 (2)
     ......
$ readelf --dyn-syms -W ./libsv.so

Symbol table '.dynsym' contains 8 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     ......
     7: 0000000000001119    23 FUNC    GLOBAL DEFAULT   15 foo@@VER_1
```

[1]: ../articles/code/20221008-symbol-version/
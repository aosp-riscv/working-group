![](./diagrams/linker-loader.png)

文章标题：**链接处理过程中的 “符号解析（Symbol Resolution）”**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. Local symbol、Global symbol 和 Symbol Resolution](#2-local-symbolglobal-symbol-和-symbol-resolution)
- [3. Global symbols 的分类](#3-global-symbols-的分类)
- [4. 符号的 Binding Type](#4-符号的-binding-type)
- [5. 符号的特性对 Symbol Resolution 的影响分析](#5-符号的特性对-symbol-resolution-的影响分析)

<!-- /TOC -->

# 1. 参考文档 

- [1] [Symbol Resolution][1]
- [2] [Symbol binding types in ELF and their effect on linking of relocatable files][2]

注: 
- 本文默认基于 ELF 格式，示例采用 C 语言，不做另外说明。
- 本文实验采用的 GCC 版本是: `gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1)`

# 2. Local symbol、Global symbol 和 Symbol Resolution

链接器在读入需要链接的 relocatable object 文件后，有一步很重要的工作就是对文件中的符号（Symbol）进行处理。

relocatable object 文件中的符号分为两大类：

第一类符号称之为 local 符号，譬如我们在 c 中采用 static 修饰的变量、函数等。这些符号只能在定义该符号文件中被访问，其他文件是无法访问的。对链接器来说，不会对 local 符号做太多的处理，默认情况下链接器从 input 文件中收集 local 符号后直接输出写入 output 文件中并保留在 `.symtab` 中，我们可以通过 strip 对 output 文件进行瘦身。

第二类符号称之为 global 符号，对于这类符号，除了定义该符号的文件本身外，其他文件也可以访问这些符号。链接器在链接过程中会对这些 global 符号进行分析和合并，因为多个文件中可能存在同名的符号，无论是对符号的引用还是定义，这个分析和合并的过程我们称之为 "Symbol Resolution"，中文翻译过来就叫 “符号解析”。链接器处理符号解析大部分情况下对我们都是无感的，但有时候也会提示 warning，严重的可能会导致 error 失败。

# 3. Global symbols 的分类

在进一步讨论符号解析之前，我们首先需要理解的是 relocatable object 文件中参与符号解析的 global 符号整体上分为以下三种类型：

- 已定义(Defined)符号：文件中创建了该符号，并且通过初始化为该符号分配了存储空间，具体还要分两种，如果初始化值为 0，则该符号编译后存放在 `.bss` section 中，如果初始值不为 0，则存放在 `.data` section 中。

- 未定义(Undefined)符号：被某个文件引用（reference），但在该文件中并未定义。

- 待定(Tentative)符号：文件中创建了该符号，但尚未初始化和分配存储空间。在 C 中体现为定义了一个未初始化的变量。这样定义的符号会被存放在一个 pseudo 的 section - “COM” 中，所以我们也把这种符号叫做 Common symbols。从链接器的角度来看，意味着有可能在另一个文件中存在一个同名的符号，需要届时合并调整。在这一点上 Tentative symbols 有点类似 Undefined symbols，但不同的是，对于 Tentative symbols，如果 linkers 在其他文件中没有找到同名的 Defined symbols，那么对原 Tentative symobls 的处理是在 output 文件中创建同名符号并将其放在 .bss section 中（意味着将在运行加载时被初始化为 0），而对于 Undefined symbosl 来说如果 linkers 无法找到同名的 Defined symbols 则将导致 linking error。

为了有助于大家理解以上概念，可以参考一个 [例子 symbols](./code/20230321-symbol-resolution-elf/symbols/)。摘录部分代码如下：

```c
extern int	u_g_bar;
extern int	u_g_foo(int);

int		t_g_bar;

int		d_g_bar = 1;
int		d_g_bar_zero = 0;
int d_g_foo()
{
	return (u_g_foo(u_g_bar));
}
```

编译后查看符号表信息，其中 Ndx 那一列和我们上面说的 symbol 的分类有关，COM 就是对应的 tentative symbols，UND 对应的是 Undefined symbols，数字的对应的是 Defined symbols，3、4、1 分别是这些 Defined symbols 对应存放的 section 的索引编号，具体值可以通过执行 `readelf -S` 获取，3 是 `.data`，4 是 `.bss`，1 是 `.text`。

```shell
readelf -s file1.o

Symbol table '.symtab' contains 19 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
   ......
    12: 0000000000000004     4 OBJECT  GLOBAL DEFAULT  COM t_g_bar
    13: 0000000000000000     4 OBJECT  GLOBAL DEFAULT    3 d_g_bar
    14: 0000000000000000     4 OBJECT  GLOBAL DEFAULT    4 d_g_bar_zero
    15: 0000000000000000    23 FUNC    GLOBAL DEFAULT    1 d_g_foo
    16: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND u_g_bar
    17: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND _GLOBAL_OFFSET_TABLE_
    18: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND u_g_foo
```

# 4. 符号的 Binding Type

Symbol resolution 处理还和符号的另外一个特性，即 Binding type 有关，这个属性体现在上面的 Bind 那一列，ELF 规范中 Binding Type 定义了好几种，我们这里关心其中主要的三种：

- LOCAL(`STB_LOCAL`)：这几乎就是对应我们前面说的 local symbols。上面例子代码中也给出了 local symbols 的情况，感兴趣可以自行查看。

- GLOBAL(`STB_GLOBAL`)：这些符号被不同对象文件共享，一个文件中的对这个符号的引用可以绑定到另一个文件中的定义。这也是我们前面说的 global symbols 的概念。

- WEAK(`STB_WEAK`)：是对 GLOBAL 的一种扩充，在 symbol solution 过程中优先级低于 GLOBAL 的符号。例如，如果链接器注意到定义了两个同名的符号，但一个是 GLOBAL 的，另一个是 WEAK 的，则会将对该符号的引用解析为 GLOBAL 的符号的值，而 WEAK 的定义将被忽略。
  可以有两种语法定义 WEAK 的符号。一种是 `#pragma weak`，还有一种是 `__attribute__((weak))`，具体例子同样参考 [例子 symbols](./code/20230321-symbol-resolution-elf/symbols/)。
  注意，对 WEAK 属性不可以再作用于 LOCAL 的符号上，这也是无意义的，因为 WEAK 本身是 GLOBAL 的一种扩充，也就是说 WEAK 属性的符号必然是 GLOBAL 的。譬如，下面的这种写法是错误的，gcc 编译会报错：`error: weak declaration of ‘w_local’ must be public`。
  ```c
  __attribute__((weak)) static int w_local = 2;
  ```

# 5. 符号的特性对 Symbol Resolution 的影响分析

总结一下：global 符号的分类（Defined/Undefined/Tentative）决定了 relocatable object 文件中该符号所存放的 section（.text/.data/.bss/COM/UND）; 符号的 Binding Type(LOCAL/GLOBAL/WEAK)决定了该符号是否会被多个 relocatable object 文件共享（看到）。

回到符号解析的问题上，在理解以上概念后，我们关心的是针对非 LOCAL 的符号，譬如 GLOBAL 和 WEAK 的符号，如果它们的符号名字相同但绑定类型不同时，各种组合情况下链接器会如何处理它们的符号解析呢？

参考 [2]，它列出了一个排列组合的表格，我这里也借鉴参考了这个表格作为总结，在此表示感谢。

表格中一共举了 19 种情况，每种情况都使用了两个文件 `file1.c` 和 `file2.c`，每个文件中都有一个同名的符号 `mysym`。表格中给出了 `file1` 和 `file2` 中符号的属性组合（Binding type 和所在的 section）和输出的链接结果 `a.out` 中解析的结果，包括最终选择了 `file1` 和 `file2` 中哪个文件中定义的符号，以及 `a.out` 中该符号的最终属性（Binding type 和所在的 section）。最后一列给出了简单的说明，更详细的说明可以参考原文 [2]。我这里给出了每个 case 的 [源码](./code/20230321-symbol-resolution-elf/)，感兴趣的同学可以实操一下。

|序号|file1 中符号的属性</br>(Binding-Type/Section)|file2 中符号的属性</br>(Binding-Type/Section)|a.out 从哪个文件中选择符号</br>(file1 OR file2)|a.out 中符号的属性</br>(Binding-Type/Section)|说明|
|----|---------------|---------------|-------|-------------|-|
| 1  | GLOBAL/.data  | GLOBAL/.bss   | N/A   | N/A         | Linker error: multiple definition of symbol|
| 2  | GLOBAL/.data  | GLOBAL/COM    | file1 | GLOBAL/.data| |
| 3  | GLOBAL/.bss   | GLOBAL/UND    | file1 | GLOBAL/.bss | |
| 4  | GLOBAL/COM    | GLOBAL/UND    | file1 | GLOBAL/.bss | file1 中未分配空间，所以在最终的 a.out 中被归类到 .bss|
| 5  | GLOBAL/UND    | GLOBAL/UND    | N/A   | N/A         | Linker error: undefined reference in both files|
| 6  | WEAK/.data    | WEAK/.bss     | file1 | WEAK/.data  | 因为两个文件中的同名符号都是 WEAK，Linker 可以自己决定选择哪个符号作为最终的定义，gcc 看上去根据命令行中输入的文件的顺序选择了第一个出现的|
| 7  | WEAK/.data    | WEAK/COM(.bss)| file1 | WEAK/.data  | 对于 file2，实际上不会为 WEAK binding 类型生成 COM 的符号，编译器会将该 WEAK 的符号放在 `.bss` section 中，所以实际结果和 6 一样|
| 8  | WEAK/.bss     | WEAK/UND      | file1 | WEAK/.bss   | |
| 9  | WEAK/COM(.bss)| WEAK/UND      | file1 | WEAK/.bss   | 对于 file1，实际上不会为 WEAK binding 类型生成 COM 的符号，编译器会将该 WEAK 的符号放在 `.bss` section 中，所以实际结果和 8 一样|
| 10 | WEAK/UND      | WEAK/UND      | N/A   | N/A         | 由于两个符号都是 UNDEFINED，而且都是 WEAK binding 类型的，所以链接不会报错，但也不会为该符号分配空间，a.out 的 section 中看不到该符号|
| 11 | GLOBAL/.data  | WEAK/.bss     | file1 | GLOBAL/.data| |
| 12 | GLOBAL/.bss   | WEAK/.data    | file1 | GLOBAL/.bss | |
| 13 | GLOBAL/.data  | WEAK/COM(.bss)| file1 | GLOBAL/.data| 对于 file2，实际上不会为 WEAK binding 类型生成 COM 的符号，编译器会将该 WEAK 的符号放在 `.bss` section 中，所以实际结果和 11 一样|
| 14 | GLOBAL/COM    | WEAK/.data    | file1 | GLOBAL/.bss | file1 中未分配空间，所以在最终的 `a.out` 中被归类到 .bss|
| 15 | GLOBAL/.bss   | WEAK/UND      | file1 | GLOBAL/.bss | |
| 16 | GLOBAL/UND    | WEAK/.bss     | file2 | WEAK/.bss   | |
| 17 | GLOBAL/COM    | WEAK/UND      | file1 | GLOBAL/.bss | file1 中未分配空间，所以在最终的 `a.out` 中被归类到 `.bss`|
| 18 | GLOBAL/UND    | WEAK/COM(.bss)| file2 | WEAK/.bss   | 对于 file2，实际上不会为 WEAK binding 类型生成 COM 的符号，编译器会将该 WEAK 的符号放在 `.bss` section 中，所以实际结果和 16 一样|
| 19 | GLOBAL/UND    | WEAK/UND      | N/A   | N/A         | Linker error: undefined reference in both files|


[1]:https://docs.oracle.com/en/operating-systems/solaris/oracle-solaris/11.4/linkers-libraries/symbol-resolution.html
[2]:https://binarydodo.wordpress.com/2016/05/12/symbol-binding-types-in-elf-and-their-effect-on-linking-of-relocatable-files/

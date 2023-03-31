![](./diagrams/linker-loader.png)

文章标题：**静态链接可执行程序的入口分析**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [参考文档](#参考文档)
- [Linux 内核中应用程序的入口](#linux-内核中应用程序的入口)
- [C 库（CRT）对入口的处理](#c-库crt对入口的处理)

<!-- /TOC -->

# 参考文档

- [1] [How statically linked programs run on Linux][2]
- [2] [About ELF Auxiliary Vectors][3]
- [3] [If you use a custom linker script, _start is not (necessarily) the entry point][6]

本文代码：
- musl：v1.2.3
- linux: v6.2

# Linux 内核中应用程序的入口

可执行程序的起点来自操作系统内核，以 Linux 为例，为了执行一个程序（program），内核首先需要通过 fork/vfork 创建一个进程（process），然后通过 exec 函数族从物理持久存储中加载（load）一个 program。

我们这里说 exec 函数族，是指这是一类函数，参考 [man 3 exec][1]：

```c
int execl(const char *pathname, const char *arg, ... /*, (char *) NULL */);
int execlp(const char *file, const char *arg, ... /*, (char *) NULL */);
int execle(const char *pathname, const char *arg, ... /*, (char *) NULL, char *const envp[] */);
int execv(const char *pathname, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execvpe(const char *file, char *const argv[], char *const envp[]);
```

这些函数只是参数形式不同，本质上最终都是调用的 Linux 内核的 `sys_execve` 这个系统调用。`sys_execve` 这个系统调用的主要工作就是根据我们给出的可执行文件在文件系统中的路径加载这个程序到内存中，在此期间会做很多工作，本文就不展开讲了。我们只需要了解进入该系统调用后，内核的大致流程如下：

```cpp
sys_execve()
-> do_execve()
-> do_execveat_common()
-> bprm_execve()
-> exec_binprm()
-> search_binary_handler()
```

内核在初始化阶段维护了一个 list，list 的元素是 `struct linux_binfmt`，每一个 `struct linux_binfmt` 对应一种内核可以处理的二进制格式 binary format，譬如 `a.out`、`ELF` 等等（注意这里 `a.out` 不是我们平时默认链接生成的可执行文件名，而是一种古老的文件格式的名称，不过现在内核中已经逐渐移除了对该格式的支持）。`search_binary_handler()` 函数会对这个 `struct linux_binfmt` 的 list 进行扫描，并尝试每个文件格式注册的 `load_binary` 回调接口，如果一个匹配成功则退出扫描。我们这里只关注 ELF 格式，该格式 [对应的 `load_binary` 回调函数][9] 是 [`load_elf_binary()`][7]。

[`load_elf_binary()`][7] 函数中大致会依次做如下工作：
- 读取并且检查程序的 ELF header。
- 根据 ELF header 加载程序的 Program header，这里面有 segment 的信息。
- 寻找 `PT_INTERP` 段（segment），如果存在说明程序是动态链接的，需要另外加载 ld。本文只考虑静态链接的情况，所以不存在 `PT_INTERP` 段。
- 根据 Program header 中有关 segments 的信息将程序从磁盘上加载（mapping）到内存中。
- [`create_elf_tables()`][10] 为程序准备好用户空间的栈，并将 sp 指向该 stack 的底部（ riscv 上是低地址方向）有关这部分工作，也属于内核和用户态程序之间的接口，下面会单独总结。
- 通过 [`START_THREAD`][8]（内部封装了`start_thread()`）设置 CPU 返回用户态时的地址，对应静态链接的程序，该地址就是程序的入口，即 ELF header 中的 `e_entry`（对于动态链接的程序，则是 ld 的入口）。很显然，`start_thread()` 宏是一个体系架构相关的函数。

内核在跳转到应用态程序入口之前为应用程序准备了很多内容。这些内容都存放在应用程序的 process stack 中。了解此时用户态 stack 的内容和布局对理解应用程序在入口处的行为很有帮助。 这个 process stack 看起来如下所示：

```
高地址
|NULL    |
|env str.|
|arg str.| 
|padding |
|AT_NULL |
|auxv[1] |
|auxv[0] |
|NULL    |
|......  |
|envp[1] |
|envp[0] |
|NULL    |
|......  |
|argv[1] |
|argv[0] |
|argc    | <--- sp
低地址
```

这个 stack 中包含的内容分成以下三类：
- argc 和 argv: 应用程序从命令行里接收的参数（argument）信息，程序名就是 argv[0]，其中 argc 是参数的个数值 + 1，argv 是一个数组，每个数组项存放一个指针，每个指针指向参数的 ascii string 的起始位置，所有的 argument 的 ascii string 都统一紧挨着放在上图的 "arg str." 区域。`argv[0]` 比较特殊，就是应用程序的名字。argv 的最后一项存放的是 NULL，用来标识 argv 数组结束以及接下来 envp 的开始。
- envp: 用来存放进程的环境（environment）变量，每个环境变量也是一个字符串，格式为 "NAME=VALUE"，所以和 argv 类似，envp 也是一个数组，每个数组项存放一个指针，每个指针指向各自对应的环境变量的 ascii string 的起始位置，所有的 environment 的 ascii string 都统一紧挨着放在上图的 "env str." 区域。envp 的最后一项存放的是 NULL，用来标识 envp 数组结束以及接下来 auxv 的开始。
- auxv: 即 ["Auxiliary Vector"][4]，用于传递一些内核的信息给应用程序。auxv 是一个数组，每个数组项是一个结构体，具体的结构体类型定义可以参考 C 库中 [`include/elf.h`][13] 的 `Elf64_auxv_t`:
  ```cpp
  typedef struct {
    uint64_t a_type;
    union {
        uint64_t a_val;
    } a_un;
  } Elf64_auxv_t;
  ``` 
  内核中并没有这个结构体类型的定义，而是定义了一个宏 [`NEW_AUX_ENT`][11] 来操作这个 auxv 的每一项。从代码中我们可以看出来和用户态的定义 `Elf64_auxv_t` 实际上是一致的，对于 64 位的系统要占 128 bit（2 个 `uint64_t`）。`Elf64_auxv_t` 本质上是一个 key-value 对，其中的 `a_type` 定义在内核的 [`include/uapi/linux/auxvec.h`][12]。用户态定义在 [`include/elf.h`][13]。

# C 库（CRT）对入口的处理

上一章节我们讲到内核通过读取 ELF header 中的 `e_entry` 值（地址）并将其设置为返回用户态的地址，这样，CPU 在返回到用户态时就会跳转到这个地址开始执行程序，这个地址可以认为就是应用程序的入口地址（“entry point”）。那么 `e_entry` 中的这个地址值是如何得到的呢。

这就是链接器的工作了，链接器我们后面叫它 linker，虽然在 gcc 里默认的链接器的名字叫 ld，这很容易和动态链接程序运行过程中使用的加载器 loader 搞混，我们这里统一把链接器称之为 linker。

具体 linker 是如何获取这个 entry point 的值，可以参考 [官方的 ld 手册有关 Entry Point 的获取方法描述][5]。在 Linux 上，GNU ld 会默认使用 `_start` 作为 entry point 对应的 symbol（有关这个的讨论可以参考 [3][6]）。

但我们知道，在我们编写应用程序（本文默认以 C 语言为例）时，我们常说程序的入口是 `main` 函数。实际上这是不准确的，如果我们只是单纯地在 C 语言的角度讨论程序的入口函数，或许可以说是 `main`，但是如果我们站在更高的维度讨论系统级别的程序入口，正如前面所述应该是 `_start`。但是很显然我们在编写 C 语言的程序时并不会自己去定义 `_start` 这个函数，那么总是有人帮我们提前定义了这个函数，而这个好心人就是 C 库。

我们可以通过运行 gcc 时加上 `-v` 选项看到在执行最后的链接步骤时，实际上除了我们自己写的 c 源文件对应的 object 文件外，gcc 还额外指定链接了一些文件，譬如 `crt1.o` 等，而这些文件是由 C 库提供的，实现了我们关心的 entry point，即 `_start` 函数。

musl 的 `crt1.o` 的源码对应为 [`crt1.c`][14], 真正的入口定义在和 ARCH 相关的 [`crt_arch.h`][15] 中，这个文件会被 [`crt1.c`][14] include。

看一下 [`crt_arch.h`][15]:

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
"mv a0, sp\n"
".weak _DYNAMIC\n"
".hidden _DYNAMIC\n\t"
"lla a1, _DYNAMIC\n\t"
"andi sp, sp, -16\n\t"
"tail " START "_c"
);
```

用户栈内核已经帮我们准备好了，这里的核心逻辑就是根据 RISC-V 的 calling conventions 将 sp 放在 a0 中（`"mv a0, sp\n"`），然后跳转到第一个 C 函数 `_start_c()`（`tail " START "_c`），这个函数定义在 [`crt1.c`][14]。也就是说参数 `p` 此时的值就是 sp 的值，作为一个 C 语言的指针，此时指向了用户态 process stack 的低地址的底部。

```cpp
void _start_c(long *p)
{
	int argc = p[0];
	char **argv = (void *)(p+1);
	__libc_start_main(main, argc, argv, _init, _fini, 0);
}
```

为加深印象，我们利用以上的知识为基础，编写一个用户态的程序，查看一下程序 main 传进来的 arg/envp/auxv，并利用 gdb 调试查看一下现场的 process stack 的实际情况是否和我们打印的一致。

代码参考 [`hello.c`][16], 参考 [《musl 交叉调试说明》][17] 搭建交叉调试环境。顺便提一下，glibc/musl 为了方便用户获取 auxv，都提供了一个叫做 `getauxval()` 的函数，但注意这个函数不是 POSIX 标准定义的。

先编译后执行一下看看打印的结果, `--help` 纯粹是为了验证命令行参数对 argc 和 argv 的影响，并无实际意义。
```shell
$ $WS/install/bin/musl-gcc -g -static hello.c -o a.out
$ qemu-riscv64 ./a.out --help
argc = 2
argv[0] = 0x0x4000800221 -> ./a.out
argv[1] = 0x0x4000800229 -> --help
envp[0] = 0x0x4000800230 -> _=/aosp/wangchen/test-qemu/install/bin/qemu-riscv64
......
envp[33] = 0x0x4000800fe0 -> SHELL=/bin/bash
AT: type = 3, value = 0x10040
...
AT: type = 31, value = 0x4000800ff0
```

然后我们上 gdb 看一下 process stack 在 entry point 位置处是否和我们上面应用程序打印的结果一致。
```shell
$ qemu-riscv64 -g 1234 ./a.out --help &
$ riscv64-unknown-linux-gnu-gdb ./a.out
```

记住在 qemu 这一侧必须要以带参数的方式启动，否则即使在 gdb 这一侧带上参数，运行时也不行, 也就是说 qemu 侧的参数是必须的，gdb 这里可以不用。

运行起来后查看内存：

```shell
$ riscv64-unknown-linux-gnu-gdb ./a.out
......
Reading symbols from ./a.out...
(gdb) target remote :1234
Remote debugging using :1234
0x0000000000010138 in _start ()
(gdb) set disassemble-next-line on
(gdb) si
0x000000000001013c in _start ()
=> 0x000000000001013c <_start+4>:       93 81 81 6c     addi    gp,gp,1736
(gdb) 
0x0000000000010140 in _start ()
=> 0x0000000000010140 <_start+8>:       0a 85   mv      a0,sp
(gdb) x /20x $sp
0x40007fffc0:   0x00000002      0x00000000      0x00800221      0x00000040
0x40007fffd0:   0x00800229      0x00000040      0x00000000      0x00000000
0x40007fffe0:   0x00800230      0x00000040      0x00800264      0x00000040
0x40007ffff0:   0x008002bc      0x00000040      0x008002d0      0x00000040
0x4000800000:   0x0080030e      0x00000040      0x00800344      0x00000040
(gdb) 
```

可见此时 sp 指向的数据：以 dword 为单位，内存中从低地址到高地址大致如下排列：
```
0x00000000-00000002 <-- argc
0x00000040-00800221 <-- argv[0]
0x00000040-00800229 <-- argv[1]
0x00000000-00000000 <-- NULL
0x00000040-00800230 <-- envp[0]
0x00000040-00800264 <-- envp[1]
......
```

我们来继续看一下 `argv[0]` 和 `argv[1]`:
```shell
(gdb) x /8c 0x0000004000800221
0x4000800221:   46 '.'  47 '/'  97 'a'  46 '.'  111 'o' 117 'u' 116 't' 0 '\000'
(gdb) x /8c 0x0000004000800229
0x4000800229:   45 '-'  45 '-'  104 'h' 101 'e' 108 'l' 112 'p' 0 '\000'        95 '_'
```

再查看一下 `envp[0]`:
```shell
(gdb) x /20c 0x0000004000800230
0x4000800230:   95 '_'  61 '='  47 '/'  97 'a'  111 'o' 115 's' 112 'p' 47 '/'
0x4000800238:   119 'w' 97 'a'  110 'n' 103 'g' 99 'c'  104 'h' 101 'e' 110 'n'
0x4000800240:   47 '/'  116 't' 101 'e' 115 's'
```


[1]:https://man7.org/linux/man-pages/man3/exec.3.html
[2]:https://eli.thegreenplace.net/2012/08/13/how-statically-linked-programs-run-on-linux
[3]:http://articles.manugarg.com/aboutelfauxiliaryvectors
[4]:https://www.gnu.org/software/libc/manual/html_node/Auxiliary-Vector.html
[5]:https://sourceware.org/binutils/docs/ld/Entry-Point.html
[6]:https://www.gridbugs.org/if-you-use-a-custom-linker-script-_start-is-not-necessarily-the-entry-point/
[7]:https://elixir.bootlin.com/linux/v6.2/source/fs/binfmt_elf.c#L818
[8]:https://elixir.bootlin.com/linux/v6.2/source/fs/binfmt_elf.c#L1342
[9]:https://elixir.bootlin.com/linux/v6.2/source/fs/binfmt_elf.c#L102
[10]:https://elixir.bootlin.com/linux/v6.2/source/fs/binfmt_elf.c#L174
[11]:https://elixir.bootlin.com/linux/v6.2/source/fs/binfmt_elf.c#L244
[12]:https://elixir.bootlin.com/linux/v6.2/source/include/uapi/linux/auxvec.h
[13]:https://git.musl-libc.org/cgit/musl/tree/include/elf.h?h=v1.2.3
[14]:https://git.musl-libc.org/cgit/musl/tree/crt/crt1.c?h=v1.2.3
[15]:https://git.musl-libc.org/cgit/musl/tree/arch/riscv64/crt_arch.h?h=v1.2.3
[16]:./code/20230404-exec-entrypoint/hello.c
[17]:./20230403-musl-build-system.md
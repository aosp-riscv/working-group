![](./diagrams/linker-loader.png)

文章标题：**musl 交叉调试说明**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 构建环境说明](#2-构建环境说明)
- [3. musl 调试版本的编译](#3-musl-调试版本的编译)
- [4. 用 qemu + gdb 调试静态链接方式下的 musl](#4-用-qemu--gdb-调试静态链接方式下的-musl)
- [5. 用 qemu + gdb 调试动态链接方式下的 musl](#5-用-qemu--gdb-调试动态链接方式下的-musl)
- [6. 用 qemu + gdb 调试动态链接库（musl 版本）](#6-用-qemu--gdb-调试动态链接库musl-版本)

<!-- /TOC -->

# 1. 参考文档

- [1] [musl 构建说明][1]
- [2] [QEMU User space emulator][2]

# 2. 构建环境说明

```shell
$ lsb_release -a
LSB Version:	core-11.1.0ubuntu2-noarch:security-11.1.0ubuntu2-noarch
Distributor ID:	Ubuntu
Description:	Ubuntu 20.04.5 LTS
Release:	20.04
Codename:	focal
$ qemu-riscv64 --version
qemu-riscv64 version 7.2.0 (v7.2.0)
Copyright (c) 2003-2022 Fabrice Bellard and the QEMU Project developers
$ riscv64-unknown-linux-gnu-gdb --version
GNU gdb (GDB) 10.1
Copyright (C) 2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
```

# 3. musl 调试版本的编译

关键是在 configure 的时候加上 `--enable-debug` 选项，即可做出调试版本的 musl libc。

以 cross compile（risv64）为例，其他步骤参考 [1]，configure 命令改成如下：

```shell
$ CROSS_COMPILE=riscv64-unknown-linux-gnu- \
../musl/configure \
--prefix=$WS/install \
--exec-prefix=$WS/install \
--syslibdir=$WS/install/lib \
--target=riscv64 \
--enable-debug
```

# 4. 用 qemu + gdb 调试静态链接方式下的 musl

目标：静态链接方式下，内核首先跳转到 crt 开始执行（在跳转到 main 之前），我们希望在 gdb 运行时程序断在 crt 的入口 （`_start`）令处，由于此时代码是汇编，我们希望可以单步进行汇编级别调试。

编译 `hello.c`（具体参考 [1]），并带上 `-g`，静态链接 `libc.a`。

```shell
$ ./install/bin/musl-gcc -g -static hello.c -o a.out
```

运行 qemu，启动本地 gdb 服务，连接端口号 1234，放到后台运行。
```shell
$ qemu-riscv64 -g 1234 ./a.out &
```

运行 gdb client
```shell
$ riscv64-unknown-linux-gnu-gdb ./a.out
......
Reading symbols from ./a.out...
(gdb)
```

然后在 gdb 的控制台命令行界面输入如下命令，即可单步汇编级别从 musl libc 的入口 `_start` 处开始调试。

```shell
(gdb) target remote localhost:1234
Remote debugging using localhost:1234
0x0000000000010138 in _start ()
(gdb) set disassemble-next-line on
(gdb) si
0x000000000001013c in _start ()
=> 0x000000000001013c <_start+4>:       93 81 81 6c     addi    gp,gp,1736
(gdb) 
```

# 5. 用 qemu + gdb 调试动态链接方式下的 musl

目标：动态链接方式下，内核会首先加载 loader/ld，我们希望在 gdb 运行时程序断在 ld 的入口 （`_dlstart`）处，同样，由于此时代码是汇编，我们希望可以单步进行汇编级别调试。

非静态方式链接和编译 `hello.c`（具体参考 [1]）非常简单。

```shell
$ ./install/bin/musl-gcc -g hello.c -o a.out
```

运行 qemu，启动本地 gdb 服务，连接端口号 1234，放到后台运行。

```shell
$ qemu-riscv64 -g 1234 ./a.out &
```
注意我们使用 qemu 运行 `a.out` 时不需要通过 `-L` 选项知会 ld 的位置。这是因为 `a.out` 中记录的 Interpreter 的路径，发现记录的是一个绝对路径 `$WS/install/lib/ld-musl-riscv64.so.1`，这也是拜我们在 configure 时设置的 `--syslibdir=$WS/install/lib` 所赐。

运行 gdb client
```shell
$ riscv64-unknown-linux-gnu-gdb ./a.out
......
Reading symbols from ./a.out...
(gdb)
```

然后在 gdb 的控制台命令行界面输入如下命令，即可单步汇编级别从 musl 的 ld 入口 `_dlstart` 处开始调试。

注意：对于 gdb 来说，在通过 target 命令连接 server 端之前我们需要通过 `set sysroot` 通知 gdb 有关 ld 的位置，因为 gdb 会利用 `set sysroot` 设置的值作为 prefix 拼上 `a.out` 中保存的 Interpreter 路径来构造得到最终的 ld 的绝对路径。而目前我们构建的 `a.out` 中记录的 Interpreter 本身已经是一个绝对路径，所以我们需要 `set sysroot` 为空，这样 gdb 就能找到我们的 ld 并加载其调试符号了。当然如果你不想调试 ld，譬如你的场景是直接从 main 开始调试，也可以不要设置 sysroot。

```shell
(gdb) set disassemble-next-line on
(gdb) show sysroot
The current system root is "/aosp/wangchen/test-gcc/install/sysroot".
(gdb) set sysroot
(gdb) show sysroot
The current system root is "".
(gdb) target remote :1234
Remote debugging using :1234
Reading symbols from /aosp/wangchen/test-musl/install/lib/ld-musl-riscv64.so.1...
0x0000004000851a0a in _dlstart () from /aosp/wangchen/test-musl/install/lib/ld-musl-riscv64.so.1
=> 0x0000004000851a0a <_dlstart+0>:     97 f1 fa ff     auipc   gp,0xfffaf
(gdb) si
```

有时候我们在 configure 阶段没有设置 `--syslibdir`, 那么 `a.out` 中记录的 Interpreter 的路径默认是 `/lib/ld-musl-riscv64.so.1`，对于这种情况，在理解了以上原理后的解决方法就很简单了，和上面的操作区别在于两点：
- 运行 qemu 时通过 `-L` 选项指定 ld 的 prefix，譬如，我们的 ld 的绝对路径是 `$WS/install/lib/ld-musl-riscv64.so.1`，那么执行 qemu 的命令行为 `qemu-riscv64 -L $WS/install -g 1234 ./a.out &`。
- 运行 gdb 时设置 sysroot，譬如设置 `set sysroot $WS/install`。


# 6. 用 qemu + gdb 调试动态链接库（musl 版本）

这里顺便记录一下

```shell
$ cat libfoo.c 
#include <stdio.h>

void foo()
{
        printf("hello from libfoo.so!\n");
}

$ cat hello.c 
#include <stdio.h>

extern void foo();
int main()
{
    printf("Hello World!\n");
    foo();
    return 0;
}
```

编译：
```shell
$ ./install/bin/musl-gcc -shared -fPIC libfoo.c -g -o $WS/install/lib/libfoo.so
$ ./install/bin/musl-gcc -g hello.c -L$WS/install/lib -lfoo
```

调试：

server 侧:
```shell
$ qemu-riscv64 -E LD_LIBRARY_PATH=$WS/install/lib -g 1234 ./a.out &
```

client 侧
```shell
$ riscv64-unknown-linux-gnu-gdb ./a.out
......
Reading symbols from ./a.out...
(gdb) set sysroot
(gdb) target remote : 1234
Remote debugging using : 1234
Reading symbols from /aosp/wangchen/test-musl/install/lib/ld-musl-riscv64.so.1...
0x0000004000851a0a in _dlstart () from /aosp/wangchen/test-musl/install/lib/ld-musl-riscv64.so.1
(gdb) b foo
Breakpoint 1 at 0x10504
(gdb) c
Continuing.
Hello World!

Breakpoint 1, foo () at libfoo.c:5
5               printf("hello from libfoo.so!\n");
(gdb) 
```


[1]:./20230401-musl-build.md
[2]:https://www.qemu.org/docs/master/user/main.html
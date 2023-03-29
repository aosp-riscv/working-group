![](./diagrams/linker-loader.png)

文章标题：**musl 构建说明**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 下载仓库](#2-下载仓库)
- [3. 构建环境说明](#3-构建环境说明)
- [4. 本地 native (x86_64) 编译（非交叉编译）](#4-本地-native-x86_64-编译非交叉编译)
- [5. 尝试 GCC 进行交叉编译(riscv64)](#5-尝试-gcc-进行交叉编译riscv64)

<!-- /TOC -->

# 1. 参考文档 

- [1] [musl wiki: Getting started][1]
- [2] [How to Use musl][2]

# 2. 下载仓库

官方版本发布页面：<https://musl.libc.org/releases.html>

- 官方源码仓库地址：<https://git.musl-libc.org/cgit/musl/>，具体下载时可以使用：`git clone git://git.musl-libc.org/musl`
- Gitee 镜像：https://gitee.com/mirrors/musl

# 3. 构建环境说明

```shell
$ lsb_release -a
LSB Version:	core-11.1.0ubuntu2-noarch:security-11.1.0ubuntu2-noarch
Distributor ID:	Ubuntu
Description:	Ubuntu 20.04.5 LTS
Release:	20.04
Codename:	focal
```

# 4. 本地 native (x86_64) 编译（非交叉编译）

采用的 x86_64 gcc 的版本：
```shell
$ gcc -v
Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-linux-gnu/9/lto-wrapper
OFFLOAD_TARGET_NAMES=nvptx-none:hsa
OFFLOAD_TARGET_DEFAULT=1
Target: x86_64-linux-gnu
Configured with: ../src/configure -v --with-pkgversion='Ubuntu 9.4.0-1ubuntu1~20.04.1' --with-bugurl=file:///usr/share/doc/gcc-9/README.Bugs --enable-languages=c,ada,c++,go,brig,d,fortran,objc,obj-c++,gm2 --prefix=/usr --with-gcc-major-version-only --program-suffix=-9 --program-prefix=x86_64-linux-gnu- --enable-shared --enable-linker-build-id --libexecdir=/usr/lib --without-included-gettext --enable-threads=posix --libdir=/usr/lib --enable-nls --enable-clocale=gnu --enable-libstdcxx-debug --enable-libstdcxx-time=yes --with-default-libstdcxx-abi=new --enable-gnu-unique-object --disable-vtable-verify --enable-plugin --enable-default-pie --with-system-zlib --with-target-system-zlib=auto --enable-objc-gc=auto --enable-multiarch --disable-werror --with-arch-32=i686 --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --with-tune=generic --enable-offload-targets=nvptx-none=/build/gcc-9-Av3uEd/gcc-9-9.4.0/debian/tmp-nvptx/usr,hsa --without-cuda-driver --enable-checking=release --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu
Thread model: posix
gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1) 
```

基本上参考 [1] 中 "Using the musl-gcc wrapper" 部分的介绍。

假设当前我们的工作目录是 `WS`

先下载仓库代码：

```shell
$ cd $WS
$ git clone git://git.musl-libc.org/musl
```

进入源码目录 checkout 到指定版本，我们这里使用 v1.2.3

```shell
$ cd musl
$ git checkout v1.2.3
```

musl 已经支持 out-of-tree 方式构建，也就是说构建过程中生成的目标文件可以不污染源码树目录，我们就采用这种方式进行配置。

```shell
$ cd $WS
$ mkdir build && cd build
$ ../musl/configure \
--prefix=$WS/install \
--exec-prefix=$WS/install \
--syslibdir=$WS/install/lib
```

- `--prefix` 指定 musl 的安装路径
- `--exec-prefix` 会指定安装 musl-gcc wrapper 的路径，musl-gcc wrapper 是一个 gcc 的封装脚本，构建好后可以将这个路径设置到 PATH 中，方便以后多次执行
- `--syslibdir`: 指定动态加载器安装的位置。指定该选项后，在使用 musl-gcc 构建可执行文件时会将这里指定的路径加入其 INTERP program header 中。值得注意的是在这个目录下生成的 musl 的动态加载器只是一个指向 `libc.so` 的符号链接，因为 musl 将 linker 和 libc 集成实现在同一个 so 中。如果这一项不给出，默认生成的 INTERP 指向 `/lib/ld-musl-x86_64.so.1`

也可以加入 `--disable-shared` 指定只编译生成静态库版本的 `libc.a`，不生成 `libc.so`，可以节省一半编译时间。

configure 完成后会在 `$WS/build` 下生成 `config.mak` 文件。

此时可以在 `$WS/build` 目录下编译并安装：
```shell
$ make && make install
```

编译后会在 build 目录下生成 obj 和 lib 目录，里面是生成的目标文件和库文件
```shell
$ ls $WS/build -l
total 12
-rw-rw-r-- 1 wangchen wangchen 1790 Mar 29 20:45 config.mak
drwxrwxr-x 2 wangchen wangchen 4096 Mar 29 20:47 lib
lrwxrwxrwx 1 wangchen wangchen   16 Mar 29 20:45 Makefile -> ../musl/Makefile
drwxrwxr-x 6 wangchen wangchen 4096 Mar 29 20:47 obj
```

安装后会在 install 目录下生成以下目录：

```shell
$ ls $WS/install/ -l
total 12
drwxr-xr-x 2 wangchen wangchen 4096 Mar 29 20:47 bin
drwxr-xr-x 9 wangchen wangchen 4096 Mar 29 20:47 include
drwxr-xr-x 2 wangchen wangchen 4096 Mar 29 20:47 lib
```

其中:
- `bin` 目录下就是 musl-gcc 这个 wrapper, 这个文件实际上是一个 shell 脚本, 内容只有一行
  ```shell
  $ cat $WS/install/bin/musl-gcc 
  #!/bin/sh
  exec "${REALGCC:-gcc}" "$@" -specs "$WS/install/lib/musl-gcc.specs"
  ```
  `${REALGCC:-gcc}` 是 shell 的一种给变量赋值的语法，参考 ["Using "${a:-b}" for variable assignment in scripts"][3]. 展开就是 `exec gcc $@ -specs "$WS/install/lib/musl-gcc.specs"`, 这使用了 gcc 支持的 "spec file" 特性，具体参考 [GCC 手册相关章节][4]。 

- `lib` 目录下是生成的 crt 文件，一些 `.a` 结尾的静态库，`libc.so`，ld 文件（实质为一个指向 `libc.so` 的符号链接），还有 `musl-gcc.specs`。
- `include` 目录下都是标准的 c 头文件，供应用编程时 include

现在我们来简单测试一下，使用 musl libc 构建一个简单的 hello 程序。

```shell
$ cd $WS
$ cat > hello.c << EOF
#include <stdio.h>
int main(int argc, char **argv) 
{ printf("hello %d\n", argc); }
EOF
$ ./install/bin/musl-gcc hello.c -o a.out
$ ./a.out
Hello World!
```

比较一下基于 musl-libc 和 glibc 静态链接的可执行程序文件的大小：

```shell
$ cd $WS
$ ./install/bin/musl-gcc -static hello.c -o musl.out
$ gcc -static hello.c -o gcc.out
$ ls -hl *.out
-rwxrwxr-x 1 wangchen wangchen 852K Mar 29 21:18 gcc.out
-rwxrwxr-x 1 wangchen wangchen  19K Mar 29 21:18 musl.out
```
是不是让 glibc 感受到了一点点压力 ;)

# 5. 尝试 GCC 进行交叉编译(riscv64)

采用的 riscv gcc 的版本：

```shell
$ riscv64-unknown-linux-gnu-gcc -v
Using built-in specs.
COLLECT_GCC=riscv64-unknown-linux-gnu-gcc
COLLECT_LTO_WRAPPER=/aosp/wangchen/test-gcc/install/libexec/gcc/riscv64-unknown-linux-gnu/11.1.0/lto-wrapper
Target: riscv64-unknown-linux-gnu
Configured with: /aosp/wangchen/test-gcc/riscv-gnu-toolchain/riscv-gcc/configure --target=riscv64-unknown-linux-gnu --prefix=/aosp/wangchen/test-gcc/install --with-sysroot=/aosp/wangchen/test-gcc/install/sysroot --with-pkgversion=g5964b5cd727 --with-system-zlib --enable-shared --enable-tls --enable-languages=c,c++,fortran --disable-libmudflap --disable-libssp --disable-libquadmath --disable-libsanitizer --disable-nls --disable-bootstrap --src=.././riscv-gcc --disable-multilib --with-abi=lp64d --with-arch=rv64imafdc --with-tune=rocket 'CFLAGS_FOR_TARGET=-O2   -mcmodel=medlow' 'CXXFLAGS_FOR_TARGET=-O2   -mcmodel=medlow'
Thread model: posix
Supported LTO compression algorithms: zlib
gcc version 11.1.0 (g5964b5cd727) 
```

先删掉 `$WS/install`
```shell
$ rm -rf $WS/install
```

然后进入 `$WS/build` 目录，做一下清理，或者干脆也删了重新来过:
```shell
$ cd $WS/build
$ make distclean
```
重新配置 `config.mak` 如下

```shell
$ CROSS_COMPILE=riscv64-unknown-linux-gnu- \
../musl/configure \
--prefix=$WS/install \
--exec-prefix=$WS/install \
--syslibdir=$WS/install/lib \
--target=riscv64
```

和前面的差别在于：

- 增加 `CROSS_COMPILE` 这个环境变量，用于指定交叉编译工具链的 triple
- 增加 `--target` 这个 configure 选项，用于指定 riscv64，默认为自动检测, 我还发现也可以不指定这个，configure 脚本会根据 `CROSS_COMPILE` 自动检测出来 target 并设置 ARCH 参数。

同样构建好后再执行 `make && make install`，依然可以采用 `./install/bin/musl-gcc` 生成 riscv 版本的 `a.out`。之所以没有什么变化，主要是拜 musl-gcc 这个封装的脚本所赐。

这里采用静态链接，方便测试。

```shell
$ ./install/bin/musl-gcc -static hello.c -o a.out
$ qemu-riscv64 ./a.out
Hello World!
```
[1]:https://wiki.musl-libc.org/getting-started.html
[2]:https://www.musl-libc.org/how.html
[3]:https://unix.stackexchange.com/questions/122845/using-a-b-for-variable-assignment-in-scripts
[4]:https://gcc.gnu.org/onlinedocs/gcc/Spec-Files.html

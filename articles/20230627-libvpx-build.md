![](./diagrams/lena.jpeg)

文章标题：**工作笔记：libvpx 构建分析**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. libvpx 及其构建](#2-libvpx-及其构建)
	- [2.1. 下载仓库：](#21-下载仓库)
	- [2.2. 非交叉构建](#22-非交叉构建)
	- [2.3. 交叉构建](#23-交叉构建)
	- [2.4. 测试](#24-测试)
	- [2.5. 文档的阅读](#25-文档的阅读)
- [3. libvpx build system 分析](#3-libvpx-build-system-分析)
	- [3.1. configure](#31-configure)
	- [3.2. make](#32-make)
	- [3.3. build 过程中如何实现 ARCH 优化函数替代](#33-build-过程中如何实现-arch-优化函数替代)

<!-- /TOC -->



# 1. 参考文档 

- libvpx 代码，版本：commit：8789421bf3ed15cd86b18b4bb8f0917fda0cccd7

# 2. libvpx 及其构建

参考 [Wikipedia - libvpx](https://en.wikipedia.org/wiki/Libvpx), 引用其描述:

> libvpx is a free software video codec library from Google and the Alliance for Open Media (AOMedia). It serves as the reference software implementation for the VP8 and VP9 video coding formats, ......

实验环境：

```bash
$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 22.04.2 LTS
Release:        22.04
Codename:       jammy

$ uname -a
Linux plct-c8 5.15.0-73-generic #80-Ubuntu SMP Mon May 15 15:18:26 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux
```

构建 libvpx 时需要安装以下两个依赖，
```bash
$ sudo apt install yasm doxygen
```

其中 yasm 用于构建 x86 版本的 libvpx 需要，doxygen 则用于构建 libvpx 的文档 docs。

## 2.1. 下载仓库：

libvpx 的官方仓库地址在 <https://chromium.googlesource.com/webm/libvpx/>。我在 github 上也创建了该仓库的 mirror：<https://github.com/aosp-riscv/libvpx>。

假设我们的工作路径是 `$WS`

```bash
$ cd $WS
$ git clone https://chromium.googlesource.com/webm/libvpx
```

## 2.2. 非交叉构建

即在 host 为 x86_64 的机器上构建 target 为 x86_64 的 libvpx

libvpx 支持 out-of-tree 构建，这样不会污染原仓库

```bash
$ cd $WS
$ mkdir build-x64
$ cd build-x64
$ ../libvpx/configure
$ make -j $(nproc)
```

默认情况下，如上方式构建的 libvpx 版本就是 host 一致的 x86_64。

## 2.3. 交叉构建

如果我们想在 host 为 x86_64 的机器上构建 target 是其他的 ARCH，譬如 arm64，则这种构建我们称之为 cross-building。libvpx 支持的 target 类型，可以通过运行如下命令获取：

```bash
$ ../libvpx/configure --help
Usage: configure [options]
Options:

Build options:
  --help                      print this message
  ......
Install options:
  ......
Advanced options:
  ......
Codecs:
  ......
Supported targets:
    arm64-android-gcc        arm64-darwin-gcc         arm64-darwin20-gcc      
    arm64-darwin21-gcc       arm64-darwin22-gcc       arm64-linux-gcc         
    arm64-win64-gcc          arm64-win64-vs15         arm64-win64-vs16        
    arm64-win64-vs16-clangcl arm64-win64-vs17         arm64-win64-vs17-clangcl
    armv7-android-gcc        armv7-darwin-gcc         armv7-linux-rvct        
    armv7-linux-gcc          armv7-none-rvct          armv7-win32-gcc         
    armv7-win32-vs14         armv7-win32-vs15         armv7-win32-vs16        
    armv7-win32-vs17        
    armv7s-darwin-gcc       
    armv8-linux-gcc         
    loongarch32-linux-gcc   
    loongarch64-linux-gcc   
    mips32-linux-gcc        
    mips64-linux-gcc        
    ppc64le-linux-gcc       
    sparc-solaris-gcc       
    x86-android-gcc          x86-darwin8-gcc          x86-darwin8-icc         
    x86-darwin9-gcc          x86-darwin9-icc          x86-darwin10-gcc        
    x86-darwin11-gcc         x86-darwin12-gcc         x86-darwin13-gcc        
    x86-darwin14-gcc         x86-darwin15-gcc         x86-darwin16-gcc        
    x86-darwin17-gcc         x86-iphonesimulator-gcc  x86-linux-gcc           
    x86-linux-icc            x86-os2-gcc              x86-solaris-gcc         
    x86-win32-gcc            x86-win32-vs14           x86-win32-vs15          
    x86-win32-vs16           x86-win32-vs17          
    x86_64-android-gcc       x86_64-darwin9-gcc       x86_64-darwin10-gcc     
    x86_64-darwin11-gcc      x86_64-darwin12-gcc      x86_64-darwin13-gcc     
    x86_64-darwin14-gcc      x86_64-darwin15-gcc      x86_64-darwin16-gcc     
    x86_64-darwin17-gcc      x86_64-darwin18-gcc      x86_64-darwin19-gcc     
    x86_64-darwin20-gcc      x86_64-darwin21-gcc      x86_64-darwin22-gcc     
    x86_64-iphonesimulator-gcc x86_64-linux-gcc         x86_64-linux-icc        
    x86_64-solaris-gcc       x86_64-win64-gcc         x86_64-win64-vs14       
    x86_64-win64-vs15        x86_64-win64-vs16        x86_64-win64-vs17       
    generic-gnu             
```

我们看到所有 target 都具备类似的范式：`<ARCH>-<OS>-<TOOLS>`，除了一个例外的 `generic-gnu`。`generic-gnu` 是当我们在执行 configure 时未指定 `--target`，同时 configure 又无法为我们自动匹配合适的 target 时最终默认的选项。我们也可以主动指定 `--target=generic-gnu`。

上一节非交叉编译例子等价于执行 `../libvpx/configure --target=x86_64-linux-gcc`。

以 arm64 为例我们可以这样交叉构建 arm64 for linux 的 libvpx 版本。

首先我们需要一个 arm64 的 cross-toolchain。我这里直接从 <https://toolchains.bootlin.com/releases_aarch64.html> 下了一个 aarch64--glibc--stable-2022.08-1.tar.bz2。

解压并设置 PATH 后确保 aarch64-linux-gcc 可以直接使用。然后执行如下命令，注意和非交叉编译的主要区别就是在 configure 命令中增加了对 CROSS 环境变量的设置。

```bash
$ cd $WS
$ mkdir build-arm64
$ cd build-arm64
$ CROSS=aarch64-linux- ../libvpx/configure --target=arm64-linux-gcc
$ make -j $(nproc)
```

这样就可以了，需要指出的是，这样做出来的 libvpx 是启用了 arm neon 指令加速的版本，这个我们在执行 configure 时的控制台打印信息中可以看出来有个明显的 "enabling neon"：
```bash
$ CROSS=aarch64-linux- ../libvpx/configure --target=arm64-linux-gcc
  enabling vp8_encoder
  enabling vp8_decoder
  enabling vp9_encoder
  enabling vp9_decoder
Configuring for target 'arm64-linux-gcc'
  enabling arm64
  enabling neon
  enabling unit_tests
  enabling webm_io
  enabling libyuv
Creating makefiles for arm64-linux-gcc libs
Creating makefiles for arm64-linux-gcc examples
Creating makefiles for arm64-linux-gcc tools
Creating makefiles for arm64-linux-gcc docs
```

arm neon 指令可以极大加速 codec 的处理速度，我们也可以不启用 neon 加速。具体构建方法如下：

```bash
$ cd $WS
$ mkdir build-arm64-generic
$ cd build-arm64-generic
$ CROSS=aarch64-linux- ../libvpx/configure --target=generic-gnu
$ make -j $(nproc)
```

观察 configure 的输出，我们会看到没有 "enabling neon"
```bash
$ CROSS=aarch64-linux- ../libvpx/configure --target=generic-gnu
  enabling vp8_encoder
  enabling vp8_decoder
  enabling vp9_encoder
  enabling vp9_decoder
Configuring for target 'generic-gnu'
  enabling generic
  enabling unit_tests
  enabling webm_io
  enabling libyuv
Creating makefiles for generic-gnu libs
Creating makefiles for generic-gnu examples
Creating makefiles for generic-gnu tools
Creating makefiles for generic-gnu docs
```

## 2.4. 测试

我们利用 libvpx 构建过程中生成的 vpxenc 工具测试一下 vp8 的 encode 效果。vpxenc 的输入是 yuv 格式文件，输出 vpx 后缀的文件（实际 payload 可以自己指定为 vp8 或者 vp9）。

yuv 的素材可以从 <http://trace.eas.asu.edu/yuv/index.html> 下载，我们使用 CIF Format(352x288) 的 <http://trace.eas.asu.edu/yuv/akiyo/akiyo_cif.7z>。

先尝试一下 x86_64 上，并对比一下开启了 codec 优化和非优化的速度区别（非优化的版本我这里不演示了，参考上面 arm64 的做法）。

```bash
$ time ./build-x64/vpxenc --codec=vp8 -w 352 -h 288 -o akiyol.vpx ./akiyo_cif.yuv 
Pass 1/1 frame  300/300   314977B    8399b/f  251981b/s 4525158 us (66.30 fps)

real    0m4.849s
user    0m4.514s
sys     0m0.036s
$ time ./build-x64-generic/vpxenc --codec=vp8 -w 352 -h 288 -o akiyol.vpx ./akiyo_cif.yuv 
Pass 1/1 frame  300/300   314977B    8399b/f  251981b/s   12265 ms (24.46 fps)

real    0m12.305s
user    0m12.267s
sys     0m0.028s
```

可见相同条件下，非优化版本（build-x64-generic）的运行速度是优化版本（build-x64）的接近 3 倍。

再来试试 arm64 的情况，因为我这里暂时没有 arm64 的真机环境，我用 qemu 来尝试，我使用的 qemu 版本是 7.2

```bash
$ time qemu-aarch64 -L $SYSROOT_ARM64 ./build-arm64/vpxenc --codec=vp8 -w 352 -h 288 -o akiyol.vpx ./akiyo_cif.yuv                                                                                                                  
Pass 1/1 frame  300/300   314977B    8399b/f  251981b/s   65437 ms (4.58 fps) 
real    1m5.578s
user    1m5.480s
sys     0m0.040s
$ time qemu-aarch64 -L $SYSROOT_ARM64 ./build-arm64-generic/vpxenc --codec=vp8 -w 352 -h 288 -o akiyol.vpx ./akiyo_cif.yuv                                                                                        Pass 1/1 frame  300/300   314977B    8399b/f  251981b/s   78780 ms (3.81 fps)

real    1m18.988s
user    1m18.876s
sys     0m0.040s
```

其中 qemu 的 -L 参数用来指定 aarch cross toolchain 中的 sysroot 路径，因为 vpxenc 是动态链接的，需要通过 -L 告诉 qemu 去哪里加载 dynamic linker。从测试数据上来看，虽然是在 qemu 上跑的，但是还是能看出来非优化版本（build-arm64-generic）的运行速度比优化版本（build-arm64）的还是要慢一些。

## 2.5. 文档的阅读

这里顺便记录一下如何阅读构建出来的 docs（html）

以上构建后，会在 build 目录下生成 libvpx 的文档，具体路径在 build 目录下的 docs 子目录下，包括 html 和 latex 两个二级子目录。这里我们介绍如何阅读 html 下的 html 文档。

```bash
$ cd $WS/build-arm64/docs/html
$ python3 -m http.server 8000 # or python -m SimpleHTTPServer 8000
```

然后可以打开浏览器，输入 URL：<http://127.0.0.1:8000/index.html> 即可访问。

# 3. libvpx build system 分析

从上一章节构建的步骤我们可以看出，基本的 libvpx 构建主要分两步：

- `../libvpx/configure ......`
- `make`

所以下面我们主要针对这两步分析其中的实现逻辑。

## 3.1. configure

configure 文件最开始会导入一个 `configure.sh`，里面定义了大量的辅助函数
```bash
. "${source_path}/build/make/configure.sh"
```

但需要注意的是，configure 文件中也会重载 `configure.sh` 中的函数，譬如 `process_cmdline()`，如果一个函数在 configure 和 `configure.sh` 中都会存在，那么实际执行的是 configure 中的。因为 bash 里面定义了同名的函数，后面定义的覆盖前面定义的。

真正的入口执行是从 configure 文件的最下面开始的
```bash
##
## END APPLICATION SPECIFIC CONFIGURATION
##
CONFIGURE_ARGS="$@"
process "$@"
print_webm_license ${BUILD_PFX}vpx_config.c "/*" " */"
cat <<EOF >> ${BUILD_PFX}vpx_config.c
#include "vpx/vpx_codec.h"
static const char* const cfg = "$CONFIGURE_ARGS";
const char *vpx_codec_build_config(void) {return cfg;}
EOF
```

所以 configure 的核心步骤就体现在 `process()` 这个函数。详细的就不过了，需要重点看的是 `process_toolchain()` 这个函数。特别重要的，这个函数中会分析 `--target` 选项：
```bash
  #
  # Set up toolchain variables
  #
  tgt_isa=$(echo ${toolchain} | awk 'BEGIN{FS="-"}{print $1}')
  tgt_os=$(echo ${toolchain} | awk 'BEGIN{FS="-"}{print $2}')
  tgt_cc=$(echo ${toolchain} | awk 'BEGIN{FS="-"}{print $3}')
```

分析得到 ISA/OS/CC 后会根据这些参数值去 enable 或者 disable 一些 features。这也是我们在 log 中看到 `enabling....` 的地方。

所有的 features 确定下来后作为配置的结果保存在 `vpx_config.h` 中，并继续影响生成 `*.mk`。

详细列举一下 `configure` 生成的结果，以 host 非交叉构建为例，`../libvpx/configure --target=x86_64-linux-gcc`, 生成如下文件：

- `config.log`： config 的日志
- `vpx_config.c`：这个文件中无非就是实现了一个 `vpx_codec_build_config()` 函数，这个函数会返回我们 执行 configure 时输入的选项。这个文件会被 libs.mk 包含。
- `vpx_config.h`: 所有配置的 0/1 形式，会被源文件包含
- `config.mk`：是 `process_targets()` 中生成的，用于 make
- `XXXX-x86_64-linux-gcc.mk`：XXXX 可能是 docs/examples/libs/tools，这些文件是 `process_targets()` 中生成的，用于 make
- `Makefile`：从 `libvpx/build/make/Makefile` 复制过来的，用于 make

## 3.2. make

执行 make 后，Makefile 首先 `include config.mk`， 这个  `config.mk` 文件是 configure 生成的，其中定义了 `ALL_TARGETS`，也就是我们构建的几个目标， 后面对应 `target`。
```makefile
ALL_TARGETS += libs
ALL_TARGETS += examples
ALL_TARGETS += tools
ALL_TARGETS += docs
```

第一次进入 Makefile 时 `target` 为空， 所以执行下面这段话：
```makefile
ifeq ($(target),)
# If a target wasn't specified, invoke for all enabled targets.
.DEFAULT:
	@for t in $(ALL_TARGETS); do \
	     $(MAKE) --no-print-directory target=$$t $(MAKECMDGOALS) || exit $$?;\
        done
....
endif
```

也就是说从 `ALL_TARGETS` 中取出小目标赋值给 `target`，再次发起 make，但这时再次进入 Makefile 时 `target` 就是有值的了。所以说同一个 Makefile 文件会在我们执行 make 时被执行两次，这个是 libvpx 的 make 构建与众不同的地方。

第二次进入 Makefile, 就会跳过上面的逻辑，走到下面的地方，

```makefile
ifneq ($(target),)
include $(target)-$(TOOLCHAIN).mk
endif
...
#
# Get current configuration
#
ifneq ($(target),)
include $(SRC_PATH_BARE)/$(target:-$(TOOLCHAIN)=).mk
endif
...
```

上面第一处 include 的这些文件就是 build 目录下在执行 configure 时生成的那些 `XXXX-x86_64-linux-gcc.mk` 文件，这些文件定义了构建某个特定的 target 时的一些通用配置。

再往下，则会再次根据 target，分别 include 源码树目录下面对应的这些 mk 文件。
- `libs.mk`
- `docs.mk`
- ...
仔细看这些 mk，基本上就明白构建每个 target 时依赖的源文件有哪些了。这些第一级的 mk 文件还会递归 include 其他 mk，譬如 `libs.mk` 中还有如下
```makefile
ifeq ($(CONFIG_VP9),yes)
  VP9_PREFIX=vp9/
  include $(SRC_PATH_BARE)/$(VP9_PREFIX)vp9_common.mk
endif
```

一个源文件是否会参与编译完全受对应的 feature 是否 enable 控制，譬如
```makefile
VP8_COMMON_SRCS-$(CONFIG_POSTPROC) += common/mfqe.c
```
则 `common/mfqe.c` 是否会参与编译就要看 `CONFIG_POSTPROC` 是否定义为 yes


## 3.3. build 过程中如何实现 ARCH 优化函数替代

以 vp8 为例，仔细阅读 `vp8/vp8_common.mk` 的最后一句话：
```makefile
$(eval $(call rtcd_h_template,vp8_rtcd,vp8/common/rtcd_defs.pl))
```
其中 `rtcd_h_template` 是一个定义在 `libs.mk` 中的 make 函数, 本质上是定义了一个 rule。

```bash
#
# Rule to generate runtime cpu detection files
#
define rtcd_h_template
$$(BUILD_PFX)$(1).h: $$(SRC_PATH_BARE)/$(2)
	@echo "    [CREATE] $$@"
	$$(qexec)$$(SRC_PATH_BARE)/build/make/rtcd.pl --arch=$$(TGT_ISA) \
          --sym=$(1) \
          --config=$$(CONFIG_DIR)$$(target)-$$(TOOLCHAIN).mk \
          $$(RTCD_OPTIONS) $$^ > $$@
CLEAN-OBJS += $$(BUILD_PFX)$(1).h
RTCD += $$(BUILD_PFX)$(1).h
endef
```

`$(1)` 就是 “vp8_rtcd”， `$(2)` 就是 "vp8/common/rtcd_defs.pl"

当我们 enable 了 vp8 这个 feature 时 `libs.mk` 会 include `vp8/vp8_common.mk`，上述 rule 执行的结果就是会在 build 目录中生成一个 `vp8_rtcd.h` 文件。生成这个文件的 action 是执行 `build/make/rtcd.pl`

观察这个生成的 `vp8_rtcd.h` 文件：
```cpp
void vp8_bilinear_predict8x4_c(unsigned char *src_ptr, int src_pixels_per_line, int xoffset, int yoffset, unsigned char *dst_ptr, int dst_pitch);
void vp8_bilinear_predict8x4_sse2(unsigned char *src_ptr, int src_pixels_per_line, int xoffset, int yoffset, unsigned char *dst_ptr, int dst_pitch);
#define vp8_bilinear_predict8x4 vp8_bilinear_predict8x4_sse2
```
其作用就是当 sse2 这个 feature 被 enable 的情况下，`vp8_bilinear_predict8x4` 会被宏替换为 `vp8_bilinear_predict8x4_sse2`，那么所有代码中调用 `vp8_bilinear_predict8x4` 的地方都会被替换为调用 `vp8_bilinear_predict8x4_sse2`。这样就实现了针对 x86_64 的 sse2 优化。

如果我们 configure 的时候没有选择 target，或者设置 target 为 `generic-gnu`, 则生成的 `vp8_rtcd.h` 文件中的对应代码如下：
```cpp
void vp8_bilinear_predict8x4_c(unsigned char *src_ptr, int src_pixels_per_line, int xoffset, int yoffset, unsigned char *dst_ptr, int dst_pitch);
#define vp8_bilinear_predict8x4 vp8_bilinear_predict8x4_c
```
也就是说 `vp8_bilinear_predict8x4` 对应的是其原始的 c 实现 `vp8_bilinear_predict8x4_c`，这时就没有优化效果了。

具体的 `vp8_bilinear_predict8x4_sse2()` 函数定义在 `vp8/common/x86/bilinear_filter_sse2.c`， 这个文件是否会参与构建同样会收到对应的 feature 是否 enable 的控制，参考 `vp8/vp8_common.mk`
```makefile
VP8_COMMON_SRCS-$(HAVE_SSE2) += common/x86/bilinear_filter_sse2.c
```

与 `vp8/common/rtcd_defs.pl` 类似的文件 libvpx 中有如下几处：

```
vp8/vp8_common.mk:
  149: $(eval $(call rtcd_h_template,vp8_rtcd,vp8/common/rtcd_defs.pl))
vp9/vp9_common.mk:
  99: $(eval $(call rtcd_h_template,vp9_rtcd,vp9/common/vp9_rtcd_defs.pl))
vpx_dsp/vpx_dsp.mk:
  472: $(eval $(call rtcd_h_template,vpx_dsp_rtcd,vpx_dsp/vpx_dsp_rtcd_defs.pl))
vpx_scale/vpx_scale.mk:
  16: $(eval $(call rtcd_h_template,vpx_scale_rtcd,vpx_scale/vpx_scale_rtcd.pl))
```

这些文件定义了那些函数会被不同的 ARCH 所优化替代，同样以 vp8_bilinear_predict8x4 为例

```perl
add_proto qw/void vp8_bilinear_predict8x4/, "unsigned char *src_ptr, int src_pixels_per_line, int xoffset, int yoffset, unsigned char *dst_ptr, int dst_pitch";
specialize qw/vp8_bilinear_predict8x4 sse2 neon msa/;
```

说明 `vp8_bilinear_predict8x4` 分别有针对 x86(sse2), arm(neon) 和 mips(msa) 的优化版本。如果没有优化替代则使用默认的 c 语言版本 `vp8_bilinear_predict8x4_c()`。




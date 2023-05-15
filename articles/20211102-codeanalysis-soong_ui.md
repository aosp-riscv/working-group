![](./diagrams/android.png)

**代码走读：对 soong_ui 的深入理解**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 构建系统对 soong_ui 的封装](#1-构建系统对-soong_ui-的封装)
	- [1.1. 第一步：准备环境](#11-第一步准备环境)
	- [1.2. 第二步：构建 soong_ui](#12-第二步构建-soong_ui)
	- [1.3. 第三步：传入指定参数，执行 soong_ui](#13-第三步传入指定参数执行-soong_ui)
- [2. `soong_ui` 程序分析](#2-soong_ui-程序分析)
	- [2.1. `soong_ui` 的 main 函数。](#21-soong_ui-的-main-函数)
	- [2.2. `soong_ui` 的参数和使用](#22-soong_ui-的参数和使用)
		- [2.2.1. `"--make-mode"`](#221---make-mode)
		- [2.2.2. `"--build-mode"`](#222---build-mode)
		- [2.2.3. `"--dumpvar-mode"`](#223---dumpvar-mode)
		- [2.2.4. `"--dumpvars-mode"`](#224---dumpvars-mode)
	- [2.3. `build.Build` 函数分析](#23-buildbuild-函数分析)

<!-- /TOC -->

注：本文的分析基于 AOSP 12 (tag: `android-12.0.0_r3`)。

# 1. 构建系统对 soong_ui 的封装

从 aosp 11 开始不支持在 AOSP 源码树下输入 make 构建，而需要替换为 `m` 或者 `build/soong/soong_ui.bash --make-mode`。参考 <https://cs.android.com/android/platform/superproject/+/android-11.0.0_r1:build/make/core/main.mk;l=1>

```makefile
ifndef KATI
$(warning Calling make directly is no longer supported.)
$(warning Either use 'envsetup.sh; m' or 'build/soong/soong_ui.bash --make-mode')
$(error done)
endif
```

而 `build/soong/soong_ui.bash` 实际上就是封装了 `soong_ui`。`soong_ui` 这个程序可以认为是 Google 在替代原先基于 make 的构建系统而引入的一个非常重要的程序，但代码中我们很难看到直接调用 `soong_ui` 这个程序的地方，更多的我们看到的是形如在 `envsetup.sh` 脚本文件中诸如 `$T/build/soong/soong_ui.bash ...` 这样的调用，这个脚本正是对 `soong_ui` 程序的封装调用，以这个脚本函数为入口， Google 将原来的以 make 为核心的框架改造为以 Soong 为核心的构建框架。

Soong 的入口封装在 `build/soong/soong_ui.bash` 这个脚本中，下面我们来看看这个脚本的核心处理逻辑，主要包括以下三步：

```bash
# 第一步：准备环境
source ${TOP}/build/soong/scripts/microfactory.bash

# 第二步：构建 soong_ui
soong_build_go soong_ui android/soong/cmd/soong_ui

# 第三步：传入指定参数，执行 soong_ui</b>
cd ${TOP}
exec "$(getoutdir)/soong_ui" "$@"
```

## 1.1. 第一步：准备环境

```bash
source ${TOP}/build/soong/scripts/microfactory.bash
```
这个被导入的脚本主要做了以下几件事情：

- 设置 GOROOT 环境变量，指向 prebuild 的 go 编译工具链
- 定义 `getoutdir()` 和 `soong_build_go()` 这两个函数。`getoutdir()` 的作用很简单，就是用于 `Find the output directory`；`soong_build_go()` 实际上是一个对 `build_go()` 函数的调用封装。`soong_build_go()` 在第二步会用到。
- 导入 `${TOP}/build/blueprint/microfactory/microfactory.bash` 这个脚本，这个脚本中定义了 `build_go()` 这个函数，这个函数的中会调用 go 的命令，根据调用的参数生成相应的程序，其中第一个参数用于指定生成的程序的名字，第二个参数用于指定源码的路径，还有第三个参数可以用于指定额外的编译参数。举个例子：`build_go soong_ui android/soong/cmd/soong_ui` 就是根据 AOSP 源码树目录 `soong/cmd/soong_ui` 的 package 生成一个可执行程序叫 `soong_ui`。

## 1.2. 第二步：构建 soong_ui

```bash
soong_build_go soong_ui android/soong/cmd/soong_ui
```

其作用是调用 `soong_build_go` 函数。这个函数有两个参数，从第一步的分析可以知道，`soong_build_go` 实际上是一个对 `build_go()` 函数的调用封装，所以以上语句等价于 `build_go soong_ui android/soong/cmd/soong_ui`。第一参数 `soong_ui` 是指定了编译生成的可执行程序的名字， `soong_ui` 是一个用 go 语言写的程序，也是 Soong 的实际执行程序。在第二个参数告诉 `soong_build_go` 函数，`soong_ui` 程序的源码在哪里，这里制定了其源码路径  `android/soong/cmd/soong_ui`（实际对应的位置是 `build/soong/cmd/soong_ui`）

综上所述，`build/soong/soong_ui.bash` 的第二步的效果就是帮助我们把 `soong_ui` 制作出来，制作好的 `soong_ui` 路径在 `out/soong_ui` 下。

p.s.: `soong_ui` 是 “soong native UI” 的缩写，这是一个运行在 host 上的可执行程序，即 Soong 的总入口。

## 1.3. 第三步：传入指定参数，执行 soong_ui

```bash
cd ${TOP}
exec "$(getoutdir)/soong_ui" "$@"
```
就是在前述步骤的基础上调用生成的· `soong_ui`, 并接受所有参数并执行，等价替换了原来的 `make $@`

综上所示，执行 `build/soong/soong_ui.bash` 本质上就是执行 `out/soong_ui`，如果该程序不存在，则 bash 脚本会保证将其制作出来，否则就直接执行。

# 2. `soong_ui` 程序分析

`soong_ui` 的主文件是 `build/soong/cmd/soong_ui/main.go` 这个文件可以认为只是 `soong_ui` 作为一个命令行程序的入口，但这个程序的内容绝对不止这一个文件。从其 `soong/cmd/soong_ui/Android.bp` 文件来看：

```blueprint
blueprint_go_binary {
    name: "soong_ui",
    deps: [
        "soong-ui-build",
        "soong-ui-logger",
        "soong-ui-terminal",
        "soong-ui-tracer",
    ],
    srcs: [
        "main.go",
    ],
}
```

编译这个 soong_ui 会涉及到以下几个依赖的 module：

- `soong/ui/build`：soong_ui 的主逻辑
- `soong/ui/logger`：Package logger implements a logging package designed for command line utilities.  It uses the standard 'log' package and function, but splits output between stderr and a rotating log file.
- `soong/ui/terminal`：Package terminal provides a set of interfaces that can be used to interact with the terminal
- `soong/ui/tracer`：This package implements a trace file writer, whose files can be opened in chrome://tracing.


## 2.1. `soong_ui` 的 main 函数。

main 函数定义在 `build/soong/cmd/soong_ui/main.go`

从 11 开始，代码有较大变化，但更加好读了

```go
// list of supported commands (flags) supported by soong ui
var commands []command = []command{
	{
		flag:        "--make-mode",
		description: "build the modules by the target name (i.e. soong_docs)",
		config: func(ctx build.Context, args ...string) build.Config {
			return build.NewConfig(ctx, args...)
		},
		stdio: stdio,
		run:   runMake,
	}, {
		flag:         "--dumpvar-mode",
		description:  "print the value of the legacy make variable VAR to stdout",
		simpleOutput: true,
		logsPrefix:   "dumpvars-",
		config:       dumpVarConfig,
		stdio:        customStdio,
		run:          dumpVar,
	}, {
		flag:         "--dumpvars-mode",
		description:  "dump the values of one or more legacy make variables, in shell syntax",
		simpleOutput: true,
		logsPrefix:   "dumpvars-",
		config:       dumpVarConfig,
		stdio:        customStdio,
		run:          dumpVars,
	}, {
		flag:        "--build-mode",
		description: "build modules based on the specified build action",
		config:      buildActionConfig,
		stdio:       stdio,
		run:         runMake,
	},
}
```

综上所述，我们知道 `soong_ui` 会接受四个参数

- "--make-mode": 对应调用 `runMake()`
- "--dumpvar-mode": 对应调用 `dumpVar()`。
- "--dumpvars-mode": 对应调用 `dumpVars()`
- "--build-mode": 对应调用 `runMake()`

下面具体分析一下这些参数。

## 2.2. `soong_ui` 的参数和使用

### 2.2.1. `"--make-mode"`

当我们输入 `m` 时就相当于调用了 `out/soong_ui --make-mode`。`--make-mode` 参数告诉 soong_ui，是正儿八经要开始编译。也就是说 `soong_ui --make-mode` 可以替代原来的 make， 所以后面还可以带一些参数选项。这些参数可能都是为了兼容 make 的习惯。

内部对应调用 `runMake` 函数，内部最终还是调用的 `build.Build(ctx, config)`


### 2.2.2. `"--build-mode"`

这个是 "--make-mode" 的升级版本，可以支持更丰富的 buildaction flags。对应调用 `runMake()`，内部最终还是调用的 `build.Build(ctx, config)`

```go
func buildActionConfig(ctx build.Context, args ...string) build.Config {
	flags := flag.NewFlagSet("build-mode", flag.ContinueOnError)
	flags.Usage = func() {
		fmt.Fprintf(ctx.Writer, "usage: %s --build-mode --dir=<path> <build action> [<build arg 1> <build arg 2> ...]\n\n", os.Args[0])
		fmt.Fprintln(ctx.Writer, "In build mode, build the set of modules based on the specified build")
		fmt.Fprintln(ctx.Writer, "action. The --dir flag is required to determine what is needed to")
		fmt.Fprintln(ctx.Writer, "build in the source tree based on the build action. See below for")
		fmt.Fprintln(ctx.Writer, "the list of acceptable build action flags.")
		fmt.Fprintln(ctx.Writer, "")
		flags.PrintDefaults()
	}

	buildActionFlags := []struct {
		name        string
		description string
		action      build.BuildAction
		set         bool
	}{{
		name:        "all-modules",
		description: "Build action: build from the top of the source tree.",
		action:      build.BUILD_MODULES,
	}, {
		// This is redirecting to mma build command behaviour. Once it has soaked for a
		// while, the build command is deleted from here once it has been removed from the
		// envsetup.sh.
		name:        "modules-in-a-dir-no-deps",
		description: "Build action: builds all of the modules in the current directory without their dependencies.",
		action:      build.BUILD_MODULES_IN_A_DIRECTORY,
	}, {
		// This is redirecting to mmma build command behaviour. Once it has soaked for a
		// while, the build command is deleted from here once it has been removed from the
		// envsetup.sh.
		name:        "modules-in-dirs-no-deps",
		description: "Build action: builds all of the modules in the supplied directories without their dependencies.",
		action:      build.BUILD_MODULES_IN_DIRECTORIES,
	}, {
		name:        "modules-in-a-dir",
		description: "Build action: builds all of the modules in the current directory and their dependencies.",
		action:      build.BUILD_MODULES_IN_A_DIRECTORY,
	}, {
		name:        "modules-in-dirs",
		description: "Build action: builds all of the modules in the supplied directories and their dependencies.",
		action:      build.BUILD_MODULES_IN_DIRECTORIES,
	}}
	// 省略 ......
}
```

这些 buildaction flag 用于实现我们常用的 m/mm/mmm 等命令，仔细看一下 `build/make/envsetup.sh` 中这些函数的定义

```bash
function m()
(
    _trigger_build "all-modules" "$@"
)

function mm()
(
    _trigger_build "modules-in-a-dir-no-deps" "$@"
)

function mmm()
(
    _trigger_build "modules-in-dirs-no-deps" "$@"
)

function mma()
(
    _trigger_build "modules-in-a-dir" "$@"
)

function mmma()
(
    _trigger_build "modules-in-dirs" "$@"
)
```


### 2.2.3. `"--dumpvar-mode"`

`"--dumpvar-mode"` 对应 `soong_ui` 的 `dumpVar()` 函数， 从代码中的 help 信息我们可以了解

```
usage: soong_ui --dumpvar-mode [--abs] <VAR>

In dumpvar mode, print the value of the legacy make variable VAR to stdout

'report_config' is a special case that prints the human-readable config banner
from the beginning of the build.
```

给个例子，执行下面命令可以打印出 `TARGET_PRODUCT` 的值，注意这些变量有可能不是 shell 的 env 变量，大部分是 make 变量，所以简单地用 `echo $XXX` 有可能是不行的。

```bash
$ ./out/soong_ui --dumpvar-mode TARGET_PRODUCT
sdk_phone64_riscv64
```
简化版本是调用 `envsetup.sh` 中预定义的函数 `get_build_var`

```bash
$ get_build_var TARGET_PRODUCT
sdk_phone64_riscv64
```

再给个例子, `report_config` 这个特殊的 VAR 会将当前选择的 product 的基本配置信息都打印出来：

```bash
$ ./out/soong_ui --dumpvar-mode report_config
============================================
PLATFORM_VERSION_CODENAME=REL
PLATFORM_VERSION=12
TARGET_PRODUCT=sdk_phone64_riscv64
TARGET_BUILD_VARIANT=eng
TARGET_BUILD_TYPE=release
TARGET_ARCH=riscv64
TARGET_ARCH_VARIANT=riscv64
TARGET_CPU_VARIANT=generic
TARGET_2ND_ARCH_VARIANT=riscv64
TARGET_2ND_CPU_VARIANT=generic
HOST_ARCH=x86_64
HOST_2ND_ARCH=x86
HOST_OS=linux
HOST_OS_EXTRA=Linux-5.4.0-100-generic-x86_64-Ubuntu-20.04.4-LTS
HOST_CROSS_OS=windows
HOST_CROSS_ARCH=x86
HOST_CROSS_2ND_ARCH=x86_64
HOST_BUILD_TYPE=release
BUILD_ID=SP1A.210812.016
OUT_DIR=out
PRODUCT_SOONG_NAMESPACES=device/generic/goldfish device/generic/goldfish-opengl hardware/google/camera hardware/google/camera/devices/EmulatedCamera device/generic/goldfish device/generic/goldfish-opengl
============================================
```

类似地，可以调用 `envsetup.sh` 中预定义的函数 `printconfig` 达到同样的效果。

### 2.2.4. `"--dumpvars-mode"`

`"--dumpvars-mode"` 对应 `soong_ui` 的 dumpVars 函数

```
usage: ./out/soong_ui --dumpvars-mode [--vars="VAR VAR ..."]

In dumpvars mode, dump the values of one or more legacy make variables, in
shell syntax. The resulting output may be sourced directly into a shell to
set corresponding shell variables.

'report_config' is a special case that dumps a variable containing the
human-readable config banner from the beginning of the build.

  -abs-var-prefix string
        String to prepent to all absolute path variable names when dumping
  -abs-vars string
        Space-separated list of variables to dump (using absolute paths)
  -var-prefix string
        String to prepend to all variable names when dumping
  -vars string
        Space-separated list of variables to dump
```

给个例子：
```bash
$ ./out/soong_ui --dumpvars-mode --vars="TARGET_PRODUCT PRODUCT_DEVICE"
TARGET_PRODUCT='sdk_phone64_riscv64'
PRODUCT_DEVICE='emulator_riscv64'
```

其实 `"--dumpvars-mode"` 和 `"--dumpvar-mode"` 的功能差不多，区别仅在于 dump 的 var 的个数多少。内部核心都是调用的 `build.DumpMakeVars()`, 具体的代码实现在 `./build/soong/ui/build/dumpvars.go`

而 `build.DumpMakeVars()` 内部最终封装的 `build.dumpMakeVars()`, 注意对于 "--make-mode"/"--build-mode" 内部如果要 BuildProductConfig 也会调用 `build.dumpMakeVars()` 这个函数。

`build.dumpMakeVars()` 这个函数就非常有趣了，看它的代码实际上是用命令行的方式去执行一个叫做 `build/make/core/config.mk` 的脚本。这个脚本是从 Android 原先的 make 系统里遗留下来的，从该文件的最前面注释上来看，原先的 Android 的 build 系统中，top-level Makefile 会包含这个 config.mk 文件，这个文件根据 platform 的不同以及一些 configration 的不同设置了一些 standard variables，这些变量`are not specific to what is being built`。

这个 `config.mk` 会 include 大量的其他 mk 文件，这些文件存放在 BUILD_SYSTEM（`./build/make/common`) 和  BUILD_SYSTEM（`./build/make/core`） 下

这其中有这么几处处理值得我们了解一下：

一处是参考的脚本文件流程 

```
build/make/core/config.mk 
-> include $(BUILD_SYSTEM)/envsetup.mk
   -> include $(BUILD_SYSTEM)/product_config.mk
      -> include $(BUILD_SYSTEM)/product.mk
```

注意 product.mk 中只是定义了一堆辅助函数，譬如 `get-all-product-makefiles` 这个函数：

```makefile
define _find-android-products-files
$(file <$(OUT_DIR)/.module_paths/AndroidProducts.mk.list) \
  $(SRC_TARGET_DIR)/product/AndroidProducts.mk
endef

define get-product-makefiles
$(sort \
  $(eval _COMMON_LUNCH_CHOICES :=) \
  $(foreach f,$(1), \
    $(eval PRODUCT_MAKEFILES :=) \
    $(eval COMMON_LUNCH_CHOICES :=) \
    $(eval LOCAL_DIR := $(patsubst %/,%,$(dir $(f)))) \
    $(eval include $(f)) \
    $(call _validate-common-lunch-choices,$(COMMON_LUNCH_CHOICES),$(PRODUCT_MAKEFILES)) \
    $(eval _COMMON_LUNCH_CHOICES += $(COMMON_LUNCH_CHOICES)) \
    $(PRODUCT_MAKEFILES) \
   ) \
  $(eval PRODUCT_MAKEFILES :=) \
  $(eval LOCAL_DIR :=) \
  $(eval COMMON_LUNCH_CHOICES := $(sort $(_COMMON_LUNCH_CHOICES))) \
  $(eval _COMMON_LUNCH_CHOICES :=) \
 )
endef

define get-all-product-makefiles
$(call get-product-makefiles,$(_find-android-products-files))
endef
```

`get-all-product-makefiles` 函数会遍历 AOSP 源码树下所有的 `AndroidProducts.mk` 文件并分析其中的 `PRODUCT_MAKEFILES` 变量和 `COMMON_LUNCH_CHOICES` 变量。

AOSP 源码树下的 `AndroidProducts.mk` 主要分布在两处，一处是 `build/make/target/product/AndroidProducts.mk` 这个文件，还有一处是在 `device` 目录下。`build/make/target/product/AndroidProducts.mk` 中存放的是 GSI 的产品定义，`device` 目录下存放的是普通产品的定义。当我们执行 lunch 命令时，所有的 `device` 目录下的 `AndroidProducts.mk` 会被提前扫描出来，列在 `out/.module_paths/AndroidProducts.mk.list` 中，但注意这里只有 `device` 目录下的， `build` 目录下的会在 mk 文件执行中自动添加，因为我们已经统一定义在 `build/make/target/product/AndroidProducts.mk`。同理 lunch 扫描 `AndroidProducts.mk` 时，会根据 `COMMON_LUNCH_CHOICES` 列出 lunch 菜单的 entry list。

`get-all-product-makefiles` 这个函数在 `product_config.mk` 中被调用如下：
```makefile
# Read in all of the product definitions specified by the AndroidProducts.mk
# files in the tree.
all_product_configs := $(get-all-product-makefiles)
```

`COMMON_LUNCH_CHOICES` 变量存放了 lunch 命令执行后列出的 product 列表，`PRODUCT_MAKEFILES` 则存放了所有我们支持的 product 的入口 mk 文件的路径。

另一处是在 `build/make/core/config.mk` 的最后 include 了这么两个文件
```makefile
ifeq ($(CALLED_FROM_SETUP),true)
include $(BUILD_SYSTEM)/ninja_config.mk
include $(BUILD_SYSTEM)/soong_config.mk
endif
```

其中 `soong_config.mk` 里将大量 Soong 需要的，但原先定义在 mk 文件中的变量打印输出到 `out/soong/soong.variables` 这个文件中，这是一个 json 格式的文件，这也是我们所谓的 dump Make Vars 的含义。dump 出来后我们就可以随时使用了。生成的 jason 语法格式为：

```
$(call add_json_str,  BuildId,                           $(BUILD_ID))
$(call add_json_val,  Platform_sdk_version,              $(PLATFORM_SDK_VERSION))
$(call add_json_csv,  Platform_version_active_codenames, $(PLATFORM_VERSION_ALL_CODENAMES))
$(call add_json_bool, Allow_missing_dependencies,        $(ALLOW_MISSING_DEPENDENCIES))
$(call add_json_list, ProductResourceOverlays,           $(PRODUCT_PACKAGE_OVERLAYS))
```

对应打印生成的例子为：
```
"BuildId": "QP1A.191105.004",
"Platform_sdk_version": 29,
"Platform_version_active_codenames": ["REL"],
"Allow_missing_dependencies": false,
"ProductResourceOverlays": ["device/generic/goldfish/overlay"],
```

这个地方对于理解 Android 中从 make 到 Soong 的转换非常重要，将这些原来定义在 make 中的变量导出为 soong variables 供新的 Soong 使用，完成了转换。

## 2.3. `build.Build` 函数分析

现在来看 `build.Build()` 这个核心函数, 源码在 `build/soong/ui/build/build.go`, 略去所有辅助的步骤，只保留核心的步骤

```go
func Build(ctx Context, config Config, what int) {
	// 省略 ......

	// config 采用 bitmask 的方式指定了 build 过程的步骤开关，指定哪些步骤做还是不做。
	// 我们可以在执行 m 命令时带入一些附加参数控制这些步骤，具体参数控制参考 `build/soong/ui/build/config.go`
	// 中的 parseArgs 函数, 具体有
	// * showcommands
	// * --skip-ninja: 跳过最后执行 runNinjaForBuild 的步骤
	// * --skip-make: 跳过 runMakeProductConfig 和 runKati*
	// * --skip-kati: 跳过 runKati*
	// * --soong-only: 跳过 runKati*，但还是会执行最后的 runNinjaForBuild
	// * --skip-soong-tests: 跳过 runSoong 中的 test 部分
	// 等等
	what := RunAll
	if config.UseBazel() {
		what = RunAllWithBazel
	}
	if config.Checkbuild() {
		what |= RunBuildTests
	}
	if config.SkipConfig() {
		ctx.Verboseln("Skipping Config as requested")
		what = what &^ RunProductConfig
	}
	if config.SkipKati() {
		ctx.Verboseln("Skipping Kati as requested")
		what = what &^ RunKati
	}
	if config.SkipKatiNinja() {
		ctx.Verboseln("Skipping use of Kati ninja as requested")
		what = what &^ RunKatiNinja
	}
	if config.SkipNinja() {
		ctx.Verboseln("Skipping Ninja as requested")
		what = what &^ RunNinja
	}

	// 省略 ......
	if what&RunProductConfig != 0 {
		// runMakeProductConfig 这个函数定义在 `build/soong/ui/build/dumpvars.go`
		// 通过调用 dumpMakeVars 将 make 系统中的一些 product 相关变量的值导出来设置给 config，
		// 为下面 runSoong/runKati/runNinja 做准备，所以代码上有注释
		// Everything below here depends on product config.
		runMakeProductConfig(ctx, config)
	}

	// Everything below here depends on product config.
	// 省略 ......
	if what&RunSoong != 0 {
		// runSoong() 这个函数定义在 `./soong/ui/build/soong.go` 中,
    		// 是 Soong 系统的重点函数！！！
		// 其最终效果就是遍历处理所有的 Android.bp 文件，转化为 ninja 描述文件，即 `./out/soong/build.ninja`
		runSoong(ctx, config)

		// 省略 ......
	}

	if what&RunKati != 0 {
		genKatiSuffix(ctx, config)
		runKatiCleanSpec(ctx, config)
		// 这里会利用 Kati 执行 `build/core/main.mk`，对所有 makefile 生成 ninja 描述文件
		// 这些 makefile 包括了还未转化为 Android.bp 的 Android.mk 以及
		// image files 的制作对应的 makefile 规则
		runKatiBuild(ctx, config)
		runKatiPackage(ctx, config)

		ioutil.WriteFile(config.LastKatiSuffixFile(), []byte(config.KatiSuffix()), 0666) // a+rw
	} else if what&RunKatiNinja != 0 {
		// Load last Kati Suffix if it exists
		if katiSuffix, err := ioutil.ReadFile(config.LastKatiSuffixFile()); err == nil {
			ctx.Verboseln("Loaded previous kati config:", string(katiSuffix))
			config.SetKatiSuffix(string(katiSuffix))
		}
	}

	// 将 Soong 和 Kati 各自生成的 ninja 描述文件合并为一个 ninja 描述文件
	// Write combined ninja file
	createCombinedBuildNinjaFile(ctx, config)

	// 省略 ......

	if what&RunNinja != 0 {
		// 省略 ......

		// 调用 ninja，根据前面 Soong 和 Kati 生成的 ninja 描述文件驱动实际的构建动作
		runNinjaForBuild(ctx, config)
	}

	// Currently, using Bazel requires Kati and Soong to run first, so check whether to run Bazel last.
	if what&RunBazel != 0 {
		runBazel(ctx, config)
	}
}
```






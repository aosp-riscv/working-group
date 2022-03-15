![](./diagrams/android.png)

**代码走读：对 soong_ui 的深入理解**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [1. 构建系统对 soong_ui 的封装](#1-构建系统对-soong_ui-的封装)
    - [1.1. 第一步：准备环境](#11-第一步准备环境)
    - [1.2. 第二步：构建](#12-第二步构建)
    - [1.3. 第三步：执行](#13-第三步执行)
- [2. `soong_ui` 程序分析](#2-soong_ui-程序分析)
    - [2.1. `soong_ui` 的 main 函数。](#21-soong_ui-的-main-函数)
    - [2.2. `soong_ui` 的参数和使用](#22-soong_ui-的参数和使用)
        - [2.2.1. `soong_ui` 的 "--dumpvar-mode" 和 "--dumpvars-mode" 参数](#221-soong_ui-的---dumpvar-mode-和---dumpvars-mode-参数)
        - [2.2.2. `soong_ui` 的 "--make-mode" 参数](#222-soong_ui-的---make-mode-参数)
    - [2.3. `build.runSoong()`](#23-buildrunsoong)

<!-- /TOC -->

注：本文的分析基于 AOSP 10。

# 1. 构建系统对 soong_ui 的封装

`soong_ui` 这个程序可以认为是 Google 在替代原先基于 make 的构建系统而引入的一个非常重
要的程序，整个构建可以说就是由这个程序驱动完成的。但代码中我们很难看到直接调用 `soong_ui` 
这个程序的地方，更多的我们看到的是形如在 `envsetup.sh` 脚本文件中诸如 
`$T/build/soong/soong_ui.bash ...` 这样的调用，这个脚本正是对 `soong_ui` 程序的封装
调用，以这个脚本函数为入口， Google 将原来的以 make 为核心的框架改造为以 Soong 为核心
的构建框架。

我们可以认为 Soong 的入口封装在 `build/soong/soong_ui.bash` 这个脚本中，下面我们来看
看这个脚本的核心处理逻辑，主要包括以下三步：

<pre>
<b>// 第一步</b>
source ${TOP}/build/soong/scripts/microfactory.bash

<b>// 第二步</b>
soong_build_go soong_ui android/soong/cmd/soong_ui

<b>// 第三步</b>
cd ${TOP}
exec "$(getoutdir)/soong_ui" "$@"
</pre>


## 1.1. 第一步：准备环境

```
source ${TOP}/build/soong/scripts/microfactory.bash
```
这个被导入的脚本主要做了以下几件事情：

- 设置 GOROOT 环境变量，指向 prebuild 的 go 编译工具链
- 定义 `getoutdir()` 和 `soong_build_go()` 这两个函数。`getoutdir()` 的作用很简单，
  就是用于 `Find the output directory`；`soong_build_go()` 实际上是一个对 `build_go()` 
  函数的调用封装。`soong_build_go()` 在第二步会用到。
- 导入 `${TOP}/build/blueprint/microfactory/microfactory.bash` 这个脚本，这个脚本
  中定义了 `build_go()` 这个函数，这个函数的中会调用 go 的命令，根据调用的参数生成相应
  的程序，其中第一个参数用于指定生成的程序的名字，第二个参数用于指定源码的路径，还有第三
  个参数可以用于指定额外的编译参数。举个例子：`build_go soong_ui android/soong/cmd/soong_ui` 
  就是根据 AOSP 源码树目录 `soong/cmd/soong_ui` 的 package 生成一个可执行程序叫 `soong_ui`。

## 1.2. 第二步：构建

```
soong_build_go soong_ui android/soong/cmd/soong_ui
```

其作用是调用 `soong_build_go` 函数。这个函数有两个参数，从第一步的分析可以知道，
`soong_build_go` 实际上是一个对 `build_go()` 函数的调用封装，所以以上语句等价于 
`build_go soong_ui android/soong/cmd/soong_ui`。第一参数 `soong_ui` 是指定了编译生
成的可执行程序的名字， `soong_ui` 是一个用 go 语言写的程序，也是 Soong 的实际执行程序。
在第二个参数告诉 `soong_build_go` 函数，`soong_ui` 程序的源码在哪里，这里制定了其源码
路径  `android/soong/cmd/soong_ui`（实际对应的位置是 `build/soong/cmd/soong_ui`）

综上所述，`build/soong/soong_ui.bash` 的第二步的效果就是帮助我们把 `soong_ui` 制作出
来，制作好的 `soong_ui` 路径在 `out/soong_ui` 下。

p.s.: `soong_ui` 是 “soong native UI” 的缩写，这是一个运行在 host 上的可执行程序，
即 Soong 的总入口。

## 1.3. 第三步：执行

```
cd ${TOP}
exec "$(getoutdir)/soong_ui" "$@"
```
就是在前述步骤的基础上调用生成的· `soong_ui`, 并接受所有参数并执行，等价替换了原来的 `make $@`


# 2. `soong_ui` 程序分析

`soong_ui` 的主文件是 `build/soong/cmd/soong_ui/main.go` 这个文件可以认为只是 `soong_ui` 
作为一个命令行程序的入口，但这个程序的内容绝对不止这一个文件。从其 `soong/cmd/soong_ui/Android.bp` 
文件来看：

```
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
- `soong/ui/logger`：Package logger implements a logging package designed for 
  command line utilities.  It uses the standard 'log' package and function, but 
  splits output between stderr and a rotating log file.
- `soong/ui/terminal`：Package terminal provides a set of interfaces that can be 
  used to interact with the terminal
- `soong/ui/tracer`：This package implements a trace file writer, whose files 
  can be opened in chrome://tracing.


## 2.1. `soong_ui` 的 main 函数。

main 函数定义在 `build/soong/cmd/soong_ui/main.go`

<pre>
func main() {
	<b>// 前面都是在做一些准备工作，譬如准备控制台和 log 等</b>
    ......

    <b>// 定义一个 build.Context 的对象， build.Context 定义参考</b>
    <b>// `soong\ui\build\context.go`</b>
    <b>// 我理解 Context 对象是一个容器，包含了在 build 过程中可能会涉及的 log，trace</b>
    <b>// 等等辅助对象, 会传给其他函数，譬如在执行过程中打印日志</b>
    buildCtx := build.Context{ContextImpl: &build.ContextImpl{
		Context: ctx,
		Logger:  log,
		Metrics: met,
		Tracer:  trace,
		Writer:  writer,
		Status:  stat,
	}}

    <b>// 下面的代码都是在为进一步解析 `--make-mode` 后面的参数做处理</b>
    <b>// 定义了一个 build.Config 类型的 config 对象</b>
    <b>// 这个 Config 对象的类定义参考 `soong/ui/build/config.go` 中的 configImpl,</b>
    <b>// 命令行中带入的各种选项参数会影响这个结构体中的成员的取值，并进而影响后面 make</b>
    <b>// 的行为。</b>
    <b>// 创建 config 时传入了 buildCtx</b>
    <b>// build.NewConfig() 这个函数的作用有一部分是会解析更多的参数</b>
    <b>// `build.NewConfig() -> build.parseArgs()`</b>
    <b>// 在 parse 的过程中一部分参数会导致设置 configImpl 中的一些成员，</b>
    <b>// 譬如 “showcommands” 会设置 c.verbose；更多的则直接加入到一个 c.arguments </b>
    <b>// 中，在处理中会直接分析，通过调用 Arguments() 函数来获得，</b>
    <b>// 例子：`if inList("help", config.Arguments()) {...}`</b>
    var config build.Config
    if os.Args[1] == "--dumpvars-mode" || os.Args[1] == "--dumpvar-mode" {
        config = build.NewConfig(buildCtx)
    } else {
        // 如果不是，我猜测就是对应的 --make-mode，则初始化时传入更多的命令行参数
        config = build.NewConfig(buildCtx, os.Args[1:]...)
    }

    <b>// 这部分略去，都是在设置一些 build output 路径等等比较次要的环境设置</b>
    <b>// 需要注意的是：</b>
    <b>// log 对应的是诸如 ./out/soong.log 这个 log 是 soong_ui 直接产生的</b>
    <b>// trace 对应的是 ./out/build.trace 因为比较大，实际都是被压缩了</b>
    <b>// status, 包含了 ./out/verbose.log 和 ./out/error.log 这些文件</b>
    <b>// 需要注意的是，根据代码注释 verbos.log 是以gz 形式保存，如果运行多次，</b>
    <b>// 则以前运行的 verbose.log 会保存为 verbose.log.#.gz</b>
    ......

    <b>// 这里会产生一些东西</b>
    <b>// build.FindSources 会创建 `out/.module_paths` 这个目录</b>
    <b>// 并会递归地搜索所有子目录下的 Android.bp 文件，并将这些文件的路径记录到</b>
    <b>// `out/.module_paths/Android.bp.list` 下去</b>
    <b>// 这个文件会在 runSoong() 时传递</b>
    f := build.NewSourceFinder(buildCtx, config)
    defer f.Shutdown()
    build.FindSources(buildCtx, config, f)

    if os.Args[1] == "--dumpvar-mode" {
        dumpVar(buildCtx, config, os.Args[2:])
    } else if os.Args[1] == "--dumpvars-mode" {
        dumpVars(buildCtx, config, os.Args[2:])
    } else {
        <b>// 这里对应的命令选项 "--make-mode"</b>
		......
        <b>// BuildAll 是一个枚举值，参考 `soong/ui/build/build.go`</b>
        <b>// 中类似如下语句：</b>
        <b>// BuildAll = BuildProductConfig | BuildSoong | BuildKati | BuildNinja</b>
        toBuild := build.BuildAll
        if config.Checkbuild() {
            <b>// 所谓 checkout 是指用户希望在 build 过程中增加额外的测试检查，</b>
            <b>// 这会导致 build 时间变长。</b>
            toBuild |= build.RunBuildTests
        }
        <b>// 譬如 m 命令时，调用 build 的  Build 函数，传进入三个主要参数：</b>
        <b>// buildCtx（上下文辅助信息），</b>
        <b>// config（配置信息，重要），</b>
        <b>// toBuild（控制整个 Build 流程关键步骤的控制参数）</b>
        build.Build(buildCtx, config, toBuild)
    }
}
</pre>

综上所述，我们知道 `soong_ui` 会接受三个参数

- "--dumpvar-mode": 对应调用 `dumpVar()`
- "--dumpvars-mode": 对应调用 `dumpVars()`
- "--make-mode": 对应调用 `build.Build()`

## 2.2. `soong_ui` 的参数和使用

`--dumpvars-mode` 和 `--dumpvar-mode` 用于 `dump the values of one or more legacy make variables`

譬如例子：
```
./out/soong_ui --dumpvar-mode TARGET_PRODUCT
```

`--make-mode` 参数告诉 soong_ui，是正儿八经要开始编译。也就是说 `soong_ui --make-mode` 
可以替代原来的 make， 所以后面还可以带一些参数选项。这些参数可能都是为了兼容 make 的习惯。

### 2.2.1. `soong_ui` 的 "--dumpvar-mode" 和 "--dumpvars-mode" 参数

"--dumpvar-mode" 对应 soong_ui 的 `dumpVar()` 函数， 从代码中的 help 信息我们可以了解

```
usage: soong_ui --dumpvar-mode [--abs] <VAR>

In dumpvar mode, print the value of the legacy make variable VAR to stdout

'report_config' is a special case that prints the human-readable config banner
from the beginning of the build.
```

"--dumpvars-mode" 对应 soong_ui 的 dumpVars 函数

```
usage: soong_ui --dumpvars-mode [--vars=\"VAR VAR ...\
In dumpvars mode, dump the values of one or more legacy make variables, in
shell syntax. The resulting output may be sourced directly into a shell to
set corresponding shell variables.

'report_config' is a special case that dumps a variable containing the
human-readable config banner from the beginning of the build.

```

这两个函数差不多，区别仅在于 dump 的 var 的个数多少。内部核心都是调用的 `build.DumpMakeVars()`, 
具体的代码实现在 `./build/soong/ui/build/dumpvars.go`

而 `build.DumpMakeVars()` 内部最终封装的 `build.dumpMakeVars()`, 注意对于 "--make-mode" 
内部如果要 BuildProductConfig 也会调用 `build.dumpMakeVars()` 这个函数。

`build.dumpMakeVars()` 这个函数就非常有趣了，看它的代码实际上是用命令行的方式去执行一
个叫做 `build/make/core/config.mk` 的脚本。

这个脚本是从 Android 原先的 make 系统里遗留下来的，从该文件的最前面注释上来看，原先的 
Android 的 build 系统中，top-level Makefile 会包含这个 config.mk 文件，这个文件根据 
platform 的不同以及一些 configration 的不同设置了一些 standard variables，这些变量 
`are not specific to what is being built`。

这个 config.mk 会 include 大量的其他 mk 文件，这些文件存放在 BUILD_SYSTEM（`./build/make/common`) 
和  BUILD_SYSTEM（`./build/make/core`） 下

注意在这个 config.mk 的最后 include 了这么两个文件
```
ifeq ($(CALLED_FROM_SETUP),true)
include $(BUILD_SYSTEM)/ninja_config.mk
include $(BUILD_SYSTEM)/soong_config.mk
endif
```

其中 `soong_config.mk` 里将大量 Soong 需要的，但原先定义在 mk 文件中的变量打印输出到 
`out/soong/soong.variables` 这个文件中，这是一个 json 格式的文件，这也是我们所谓的 
dump Make Vars 的含义。dump 出来后我们就可以随时使用了。生成的 jason 语法格式为：

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

这个地方对于理解 Android 中从 make 到 Soong 的转换非常重要，看上去 Android 的思路还是
先保留了原先 Make 的一套核心的 setup 逻辑，然后导出为 soong variables 供新的 Soong 
使用，完成了转换。

### 2.2.2. `soong_ui` 的 "--make-mode" 参数

现在来看 `build.Build()` 这个核心函数, 源码在 `./soong/ui/build/build.go`, 略去所有
辅助的步骤，只保留核心的步骤

<pre>
func Build(ctx Context, config Config, what int) {
    ......

    <b>// what 传入的是 BuildAll = BuildProductConfig | BuildSoong | BuildKati | BuildNinja</b>
    <b>// runMakeProductConfig 这个函数定义在 `./soong/ui/build/dumpvars.go`</b>
    <b>// 对 config 设置环境变量，为下面 runKati/runNinja 做准备</b>
    <b>// 具体的 runMakeProductConfig() 逻辑看上一节有关 dumpMakeVars 的分析</b>
    if what&BuildProductConfig != 0 {
        // Run make for product config
        runMakeProductConfig(ctx, config)
    }

    ......

    <b>// runSoong() 这个函数定义在 `./soong/ui/build/soong.go` 中,</b>
    <b>// 是 Soong 系统的重点函数！！！</b>
    if what&BuildSoong != 0 {
        // Run Soong
        runSoong(ctx, config)
    }
    if what&BuildKati != 0 {
        // Run ckati
        // ......
    }
    // Write combined ninja file
    createCombinedBuildNinjaFile(ctx, config)
    if what&RunBuildTests != 0 {
        testForDanglingRules(ctx, config)
    }
    if what&BuildNinja != 0 {
        if !config.SkipMake() {
            installCleanIfNecessary(ctx, config)
        }
        // Run ninja
        runNinja(ctx, config)
    }
}
</pre>

对 Build 这个核心函数的分析来看，其实最重要的是 `runSoong()`, 这个函数最终生成了 
`./out/soong/build.ninja`, `runNinja()` 啥的都是以这个最终的 ninja 文件作为输入，在
这个基础上执行编译构建的工作。

## 2.3. `build.runSoong()`

参考 `./soong/ui/build/soong.go`

这个函数中的每一步，都要搞清楚，否则没法彻底搞清楚基于 Soong 的 Android 构建过程。


<pre>
func runSoong(ctx Context, config Config) {
	ctx.BeginTrace(metrics.RunSoong, "soong")
	defer ctx.EndTrace()

    <b>// 本质上就是执行 `build/blueprint/bootstrap.bash -t` 这个脚本， “-t“ 表示执行测试</b>
    <b>// BOOTSTRAP/BLUEPRINTDIR，我们获得了 blueprint 的源码位置</b>
    <b>// BUILDDIR 确定了生成输出的二进制程序的位置在 out/soong 目录下</b>
    <b>// NINJA_BUILDDIR 存放的是生成 .ninja_log/.ninja_deps 的位置</b>
    <b>// GOROOT 指向 go 编译器的位置</b>
    <b>// 查看 `build/blueprint/bootstrap.bash` 其主要工作流程：</b>
    <b>// - 创建目录 $BUILDDIR/.minibootstrap，</b>
    <b>// - 在上面创建的目录下创建一系列文件，最主要的包括</b>
    <b>//   - $BUILDDIR/.minibootstrap/build.ninja：这个文件的内容很关键，</b>
    <b>//     是生成下一个阶段 bootstrap 的 目标的 ninja build 文件</b>
    <b>//   - $BUILDDIR/.minibootstrap/build-globs.ninja: 内容为空</b>
    <b>//   - $BUILDDIR/.blueprint.bootstrap:</b>
    <b>//   - ${BUILDDIR}/.out-dir</b>
    <b>// - 拷贝了一个 $WRAPPER 到 $BUILDDIR 下, 具体的这个变量为空，所以没有什么动作</b>
    func() {
        ctx.BeginTrace(metrics.RunSoong, "blueprint bootstrap")
        defer ctx.EndTrace()
        cmd := Command(ctx, config, "blueprint bootstrap", "build/blueprint/bootstrap.bash", "-t")
        cmd.Environment.Set("BLUEPRINTDIR", "./build/blueprint")
        cmd.Environment.Set("BOOTSTRAP", "./build/blueprint/bootstrap.bash")
        cmd.Environment.Set("BUILDDIR", config.SoongOutDir())
        cmd.Environment.Set("GOROOT", "./"+filepath.Join("prebuilts/go", config.HostPrebuiltTag()))
        cmd.Environment.Set("BLUEPRINT_LIST_FILE", filepath.Join(config.FileListDir(), "Android.bp  list"))
        cmd.Environment.Set("NINJA_BUILDDIR", config.OutDir())
        cmd.Environment.Set("SRCDIR", ".")
        cmd.Environment.Set("TOPNAME", "Android.bp")
        cmd.Sandbox = soongSandbox
        cmd.RunAndPrintOrFatal()
    }()

    <b>// 环境检查， 运行了一个 soong_env 的 程序, 实际测试好像也没有生成，也没有运行</b>
    func() {
        ctx.BeginTrace(metrics.RunSoong, "environment check")
        defer ctx.EndTrace()
        envFile := filepath.Join(config.SoongOutDir(), ".soong.environment")
        envTool := filepath.Join(config.SoongOutDir(), ".bootstrap/bin/soong_env")
        if _, err := os.Stat(envFile); err == nil {
            if _, err := os.Stat(envTool); err == nil {
                cmd := Command(ctx, config, "soong_env", envTool, envFile)
                cmd.Sandbox = soongSandbox
                var buf strings.Builder
                cmd.Stdout = &buf
                cmd.Stderr = &buf
                if err := cmd.Run(); err != nil {
                    ctx.Verboseln("soong_env failed, forcing manifest regeneration")
                    os.Remove(envFile)
                }
                if buf.Len() > 0 {
                    ctx.Verboseln(buf.String())
                }
        	} else {
        	    ctx.Verboseln("Missing soong_env tool, forcing manifest regeneration")
        	    os.Remove(envFile)
        	}
        } else if !os.IsNotExist(err) {
            ctx.Fatalf("Failed to stat %f: %v", envFile, err)
        }
    }()

    var cfg microfactory.Config
    cfg.Map("github.com/google/blueprint", "build/blueprint")

    cfg.TrimPath = absPath(ctx, ".")

    <b>// 利用 blueprint 的 microfactory 创建 minibp 这个可执行程序</b>
    <b>// minibp 的源码在 `build/blueprint/bootstrap/minibp`</b>
    func() {
    	ctx.BeginTrace(metrics.RunSoong, "minibp")
    	defer ctx.EndTrace()
    	minibp := filepath.Join(config.SoongOutDir(), ".minibootstrap/minibp")
    	if _, err := microfactory.Build(&cfg, minibp, "github.com/google/blueprint/bootstrap/minibp")    err != nil {
    	    ctx.Fatalln("Failed to build minibp:", err)
    	}
    }()

    <b>// 利用 blueprint 的 microfactory 创建 bpglob 这个可执行程序</b>
    <b>// bpglob 的源码在 `build/blueprint/bootstrap/bpglob`</b>
    func() {
        ctx.BeginTrace(metrics.RunSoong, "bpglob")
        defer ctx.EndTrace()
        bpglob := filepath.Join(config.SoongOutDir(), ".minibootstrap/bpglob")
        if _, err := microfactory.Build(&cfg, bpglob, "github.com/google/blueprint/bootstrap/   pglob")    err != nil {
            ctx.Fatalln("Failed to build bpglob:", err)
        }
    }()

    <b>// 这里是定义一个匿名函数 ninja</b>
    ninja := func(name, file string) {
        ctx.BeginTrace(metrics.RunSoong, name)
        defer ctx.EndTrace()
       	fifo := filepath.Join(config.OutDir(), ".ninja_fifo")
        nr := status.NewNinjaReader(ctx, ctx.Status.StartTool(), fifo)
        defer nr.Close()
       	    cmd := Command(ctx, config, "soong "+name,
            config.PrebuiltBuildTool("ninja"),
            "-d", "keepdepfile",
            "-w", "dupbuild=err",
            "-j", strconv.Itoa(config.Parallel()),
            "--frontend_file", fifo,
            "-f", filepath.Join(config.SoongOutDir(), file))
        cmd.Sandbox = soongSandbox
        cmd.RunAndPrintOrFatal()
    }

    <b>// 利用 ninja，输入 `.minibootstrap/build.ninja`, 输出 `out/soong/.bootstrap/build.ninja`</b>
    <b>// 至此可以认为 minibootstrap 阶段结束</b>
    ninja("minibootstrap", ".minibootstrap/build.ninja")

    <b>// 利用 ninja，输入 `out/soong/.bootstrap/build.ninja`， 输出  `out/soong/build.ninja`</b>
    ninja("bootstrap", ".bootstrap/build.ninja")
}
</pre>


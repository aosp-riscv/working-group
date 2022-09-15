![](./diagrams/android.png)

文章标题：**Android Init Language 学习笔记**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>


本文为对 Andorid 系统中启动阶段 second stage init 执行的 `.rc` 文件相关知识点的笔记整理。

基于 AOSP 12 (tag: `android-12.0.0_r3`)，主要参考了 AOSP 源码树中的 `system/core/init/README.md` 文件， 结构上根据自己的理解做了调整和取舍。


文章大纲

<!-- TOC -->

- [1. Init `.rc` 文件](#1-init-rc-文件)
	- [1.1. Android 加载 `.rc` 文件的方式](#11-android-加载-rc-文件的方式)
	- [1.2. Android 定义 `.rc` 文件的方式](#12-android-定义-rc-文件的方式)
- [2. init `.rc` 文件的写法](#2-init-rc-文件的写法)
	- [2.1. 基本语法](#21-基本语法)
	- [2.2. statements](#22-statements)
		- [2.2.1. Imports](#221-imports)
		- [2.2.2. Actions](#222-actions)
		- [2.2.3. Commands](#223-commands)
		- [2.2.4. Services](#224-services)
		- [2.2.5. Options](#225-options)

<!-- /TOC -->

# 1. Init `.rc` 文件

在系统启动阶段，运行到 second stage init 时会对文件系统进行进一步的定制，譬如创建文件等，同时会启动一些后台服务。这些行为统统定义在后缀名为 `.rc` 的文件种。

而这些 `.rc` 文件虽然是普通的文本文件，但文件种的内容也是有格式的，这就是采用了所谓的 Android Init Language，简称 AIL。具体语法在后面总结。

## 1.1. Android 加载 `.rc` 文件的方式

需要注意的是：在 Android 系统中目前并没有定义一个统一的 rc 文件。而是存在多个 rc 文件，分布在系统的多个地方。在 second stage init 执行过程中会先读入并 Parse 这些文件，然后统一进行处理，具体的逻辑可以参考 `system/core/init/init.cpp` 中的 `SecondStageMain()` 函数，加载读入所有 `.rc` 文件的处理在 `LoadBootScripts()` 函数中。`LoadBootScripts()` 函数的整体效果可以引用 `system/core/init/README.md` 上给出的一段伪代码来理解：

```
    fn Import(file)
      Parse(file)
      for (import : file.imports)
        Import(import)

    Import(/system/etc/init/hw/init.rc)
    Directories = [/system/etc/init, /system_ext/etc/init, /vendor/etc/init, /odm/etc/init, /product/etc/init]
    for (directory : Directories)
      files = <Alphabetical order of directory's contents>
      for (file : files)
        Import(file)
``` 

该段伪代码告诉我们以下几个概念：

首先，这些 rc 文件中存在一个主（primary） rc 文件，其位置为 `/system/etc/init/hw/init.rc`，这是 target 上文件系统路径，在 AOSP 源码树中的路径是 `system/core/rootdir/init.rc`。这个 primary rc 文件会被第一个读入。

其次，除了这个 primary rc 文件外，second stage init 会在一些固定目录下搜索 rc 文件，这些目录如下，注意搜索的顺序也是固定的，同一个目录下的 rc 文件会按照文件名排序后依次读入：

- `/system/etc/init/*.rc`
- `/system_ext/etc/init/*.rc`
- `/vendor/etc/init/*.rc`
- `/odm/etc/init/*.rc`
- `/product/etc/init/*.rc`

注意这个搜索和导入的顺序会影响同一类 action 的执行。原因是 init 在读入并解析 rc 文件过程中，会维护一个队列。按照读入和 parse 的顺序规则采用 “先来先处理” 的方式对满足同一类条件的 action 依次进行处理。也就是说如果当 trigger 条件满足时，定义在不同 rc 文件中的同一种 action 会按照导入的顺序依次执行。

之所以分目录存放，从第一级目录名称，譬如 "system/vendor/odm" 基本可以看出其设计目的，引用 `system/core/init/README.md`：

>   1. /system/etc/init/ is for core system items such as
>      SurfaceFlinger, MediaService, and logd.
>   2. /vendor/etc/init/ is for SoC vendor items such as actions or
>      daemons needed for core SoC functionality.
>   3. /odm/etc/init/ is for device manufacturer items such as
>      actions or daemons needed for motion sensor or other peripheral
>      functionality.


此外，如果 rc 文件中还存在 `import <path>` 语句，import 的作用其实也还是读入并解析，所以一旦出现 `import <path>` 就会发生递归的读入操作。从伪代码中的 `Import()` 函数的定义方式我们可以知道，如果一个 rc 文件，譬如 `/system/etc/init/hw/init.rc` 中又 `import /init.environ.rc`，那么 `/init.environ.rc` 中定义的同类 action 的执行顺序是在 `/system/etc/init/hw/init.rc` 之后的。

## 1.2. Android 定义 `.rc` 文件的方式

基于模块化的原则，Andorid 建议每个后台服务会对应提供一个 init rc 文件，譬如我们在 `/system/etc/init` 下就会看到一堆 rc 文件，除了 每个 rc 文件大致对应一个服务。

以 logcatd 为例， 其对应的 rc 文件为 `/system/etc/init/logcatd.rc`

AOSP 源码树中 logcat 的 bp 文件 `system/logging/logcat/Android.bp` 通过 `init_rc` 属性指定了 rc 文件名称。默认指示 `logcatd.rc` 放在 `/system/etc/init/logcatd.rc`

```json
init_rc: ["logcatd.rc"],
```
如果采用 `Android.mk`，则通过 `LOCAL_INIT_RC` 进行定义。

# 2. init `.rc` 文件的写法

`.rc` 文件中的文本遵循 Android Init Language 格式，简称 AIL，规范定义在 `system/core/init/README.md` 中。

## 2.1. 基本语法

- 以行为单位的，各种记号（token）之间由空格来隔开。
- 双引号也可用于防止在 parse 过程中字符串被空格分割成多个记号。譬如 `"sub-token1 sub-token2"`
- 如果一行要写成两个文本行，可以在行末的添加反斜杠方式，类似 C 语言中
- 注释行以 `#` 开头

## 2.2. statements

整个 `.rc` 文件的基本组织单元在 AIL 中称之为 statement。一个 statement 可以只有一行，也可以是由多行组成。statement 是 `.rc` 文件中可以存在的一个最小的完整的单元。每个 `.rc` 中至少含有一个或者多个 statement。

statement 分为五种类型，分别为 Actions, Commands, Services, Options, 和 Imports。

### 2.2.1. Imports

Imports 语句比较简单，就是一行。写起来有点像 C 语言中的 `#include` 的效果。但根据我们在前面 "Android 加载 `.rc` 文件的方式" 部分的介绍知道，其效果和 `#include` 不是一回事。

例子：参考 `system/core/rootdir/init.rc`

```
import /init.environ.rc
import /system/etc/init/hw/init.usb.rc
import /init.${ro.hardware}.rc
import /vendor/etc/init/hw/init.${ro.hardware}.rc
import /system/etc/init/hw/init.usb.configfs.rc
import /system/etc/init/hw/init.${ro.zygote}.rc

```

### 2.2.2. Actions

语法形式如下：

```
on <trigger> [&& <trigger>]*
   <command>
   <command>
   <command>
```

Actions 是 `.rc` 文件的核心，所有的其他 Commands, Services, Options 可以认为都是围绕 Actions 语句展开的。Commands 就是 Actions 中的 `<command>`。某些特殊的 Command 会触发一些服务，Services 就是用来定义服务的，而 Options 会在定义服务时用到。具体 Commands, Services, Options 在后面再介绍。

从语法上看，一个 Action 作为一个整体，定义了当某种 trigger 条件满足时，触发一系列的 command 被执行。可以存在多个条件 trigger，用 `&&` 连接表示 “且” 的关系。

init 在读入并解析 rc 文件过程中，会维护一个队列。按照 import 的顺序规则采用 “先来先处理” 的方式对满足同一类条件的 action 依次进行处理。README 上有个例子，可以看一下。

而 trigger 作为一种条件，在具体实现时表现为两种形式：event trigger 和 property trigger：

- event trigger，形式上体现为一个字符串，譬如 "early-init" 等，例子：`system/core/rootdir/init.rc` 中

  ```
  on early-init
    # Disable sysrq from keyboard
    write /proc/sys/kernel/sysrq 0
  ```

  event trigger 可以通过两种方式触发，一种是利用 "trigger" command, 譬如, `system/core/rootdir/init.rc` 中

  ```
  on late-init
    trigger early-fs
  ```

  或者在代码中通过调用 `QueueEventTrigger()` 函数，譬如 `system/core/init/init.cpp` 中：

  ```cpp
  int SecondStageMain(int argc, char** argv) {
	// 省略 ......
	am.QueueEventTrigger("early-init");
	// 省略 ......
  ```

- property trigger，用于表示 property 发生变化。具体又有两种形式：
  
  - `property:<name>=<value>`: 表示名字为 `<name>` 的 property 的值变成一个指定的值 `<value>`
  - `property:<name>=\*`: 表示名字为 `<name>` 的 property 的值发生变化，至于变成啥不用关心，只要发生变化就认为条件发生

以上两种方式的 trigger 可以存在 && 的关系构成组合 trigger，但是语法规定当存在组合情况时，event trigger 最多只能有一个，而 property trigger 可以有多个，合法的例子如下：

```
on boot && property:ro.config.low_ram=true
    ......
on zygote-start && property:ro.crypto.state=encrypted && property:ro.crypto.type=file
    ......
on property:security.perf_harden=0 && property:sys.init.perf_lsm_hooks=""
    ......
```


### 2.2.3. Commands

Commands是 AIL 预定义的字符串组合，具体查 `system/core/init/README.md`

值得注意的是 `start <service>` 和 `stop <service>` 这些命令中的 `<service>` 就是通过 Service 定义的服务的 name。

### 2.2.4. Services

定义 service，每一个 service 通过 fork 方式生成为 init 的子进程，所以这些 service 进程的 PPID 值都为 1。

Services 的语法定义
```
service <name> <pathname> [ <argument> ]*
   <option>
   <option>
   ...
```
其中：
- 第一行的 service 为关键字，表示定义 Service 的 statement 开始
- name ：表示此服务的名称，也就是在 start 或者 stop 等命令后面跟的 `<service>`
- pathname ：此服务对应的可执行程序在 target 文件系统中的路径
- argument ：启动服务所带的参数
- option ：对此服务的约束选项， 通过 Options statement 定义

例如： `service servicemanager /system/bin/servicemanager` 代表的是服务名为 servicemanager，服务执行的路径为 `/system/bin/servicemanager`。

Services 只是定义服务，具体启动服务，通过 `start <service>`，其中 `<service>` 为服务的 name。这个在 Commands 部分已经讲过。

对于同一个程序，也就是同一个 `<pathname>`，可以定义多个服务，通过不同的 `<option>` 进行区分，譬如

```
service service-name-1 pathname
    <option 1>

service service-name-2 pathname
    <option 2>
```

可以在命令行中查看一个服务的状态，具体是执行 `getprop init.svc.<name>`，其中 `<name>` 为一个 service 的 name。返回值是 "stopped", "stopping", "running", "restarting"。例子：

```bash
# getprop init.svc.logd
running
```

### 2.2.5. Options

Options 用于对 Services 进行进一步的配置，是 Services 的属性，见 Services 语法格式的 `<option>` 部分.

`<option>` 是由 AIL 预定义的，和 `<command>` 一样，具体要参阅 `system/core/init/README.md`，譬如

- capabilities
- class
- ......




![](./diagrams/android-riscv.png)

文章标题：**Android Dynamic Linker 之 find_libraries()**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

Android 的 bionic 仓库中实现了 Android 系统的 Dynamic linker（bionic 中也简称 linker，本文中如果提及 linker 也指 dynamic linker）。

linker 的具体代码在 `<AOSP>/bionic/linker` 下。下文以 `<AOSP>` 指代 aosp 采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

`find_libraries`()` 这个函数是 Android 中 linker 和 libdl 中 实现加载库的核心函数，该函数实现了加载动态库，重定位符号的主要过程。整体描述在另一篇总结 [《Android Linker 总览》][1] 中的 “load 和 link 的具体实现” 章节。

# `find_libraries`()` 函数

`find_libraries`()` 这个函数定义在 `<AOSP>/bionic/linker/linker.cpp`。

这个函数本身比较长，但步骤很清晰，代码注释中将该函数的逻辑总结为 7 个步骤，我们就结合这 7 个步骤来看一下，这里简单总结一下

- Step 0: prepare. 

  主要是创建了一个 `LoadTaskList load_tasks`，这是一个 LoadTask 的 list， 一个 LoadTask 感觉对应一个需要 load 的 so，取这个名字感觉仅仅是为了和一个实际的 so 文件有所区别，也可以认为该对象用于存放对应 `one corresponding loading so` 所需要的所有上下文信息，这个对象会被传给譬如 `find_library_internal()` 这些实际的动作函数去执行

- Step 1: expand the list of load_tasks to include all DT_NEEDED libraries (do not load them just yet)
  
  遍历 load_tasks 中的每一个 LoadTask
  
  - 如果这个 LoadTask 是一个被其他 so 依赖，但这个 so 不是 start_with，则标记之为 `dt_needed`
  - 对这个 LoadTask 调用 `find_library_internal()`， 这个函数内部没有太多的操作逻辑，核心就是反复尝试调用 `load_library()` 这个函数（有关这个函数我们后面会重点分析一下）。对该 so 执行实际的 find -> open -> load 的操作。注意如果这个 so 还依赖其他的 so 则 `find_library_internal()` 会把其依赖的 so 也加入 load_tasks，这应该就是注释中所谓的 expand。 注释中说的 `do not load them just yet` 意思是这里 `load_library()` 中只会调用 `LoadTask::read()` 成员读入部分内容，真正的 load 体现在 Step 2 中调用 `LoadTask::load()`, 这时才会加载 ELF 的 segment 信息。
  - 尝试获取 ld_preloads， FIXME 貌似不是重点操作
  - Add the new global group members to all initial namespaces. FIXME 没看懂在干什么

- Step 2: Load libraries in random order 
  创建了一个 `LoadTaskList load_list;` 将 load_tasks 中的对象加入这个新的 list
  实现 “ANDROID_DLEXT_RESERVED_ADDRESS_RECURSIVE” 特性 FIXME
  Set up address space parameters.
  遍历 load_list，对每个 LoadTask 调用 `LoadTask::load()` 函数，传入 address_space，完成实际的加载到内存。

- Step 3: pre-link all DT_NEEDED libraries in breadth first order.
  基于 load_tasks 基于 BF 方式对每个 soinfo 
  - 调用 prelink_image()
  - register_soinfo_tls()  FIXME 这个函数在 TLS 中还要再分析一下

- Step 4: Construct the global group. DF_1_GLOBAL bit is force set for LD_PRELOADed libs because they must be added to the global group. Note: The DF_1_GLOBAL bit for a library is normally set in step 3.

- Step 5: Collect roots of local_groups.

- Step 6: Link all local groups
  执行 relocation

- Step 7: Mark all load_tasks as linked and increment refcounts for references between load_groups

# `load_library()` 函数

`load_library()` 函数被重载了两次，两个原型如下，我这里标记其为 TYPE I 和 TYPE II：
- TYPE I:  `static bool load_library(android_namespace_t*, LoadTask*, LoadTaskList*, int, const std::string&, bool);`
- TYPE II: `static bool load_library(android_namespace_t*, LoadTask*, ZipArchiveCache*, LoadTaskList*, int, bool);`

从代码上看是如下关系：
- `find_library_internal()` 调用 TYPE II 的 `load_library()`
- TYPE II 的 `load_library()` 内部调用了 TYPE I 的 `load_library()`

所以认为 TYPE II 封装了 TYPE I

TYPE II 的 `load_library()` 的主体逻辑是
- 先调用 `open_library()` 尝试 open so，搜索的优先次序算法在 `open_library()` 中定义
- 如果 open 成功，`open_library()` 返回打开的文件的 fd，然后调用 TYPE I 的 `load_library()` 执行进一步的 load 动作
  
TYPE I 的 `load_library()` 分析：
- 前面做了很多的检查，非常严格，直到调用 `soinfo_alloc()`
- 调用 `soinfo_alloc()` 为 so 创建 siinfo 对象, 
  - 开始时会打印 `TRACE("name %s: allocating soinfo for ns=%p", name, ns);` 
  - 为该 so 创建一个 soinfo 对象并加入 solist，这里要涉及分配内存
  - 结束时打印 `TRACE("name %s: allocated soinfo @ %p", name, si);` 注意这里 @ 的地址是分配的 soinfo 对象的地址
- Read the ELF header and some of the segments.
- Find and set DT_RUNPATH, DT_SONAME, and DT_FLAGS_1.
- 最后会把当前这个 so 依赖的 so 也加入 `load_tasks`

FIXME：后面可以结合 log 的信息把这里的步骤丰富一下。

[1]:./20221220-android-linker-overview.md
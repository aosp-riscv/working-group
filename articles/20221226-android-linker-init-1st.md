![](./diagrams/android-riscv.png)

文章标题：**Android Dynamic Linker 初始化流程的第一阶段处理**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

Android 的 bionic 仓库中实现了 Android 系统的 Dynamic linker（bionic 中也简称 linker，本文中如果提及 linker 也指 dynamic linker）。

linker 的具体代码在 `<AOSP>/bionic/linker` 下。下文以 `<AOSP>` 指代 aosp 采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

在笔记 [《Android Dynamic Linker 的入口》][1]中我们知道当我们在 Android 上执行一个动态链接的应用程序时，操作系统会首先加载 dynamic linker 并跳转到 dynamic linker 的入口函数 `_start()` 处开始执行，这个函数实际上没干啥，直接再次调用 `__linker_init()` 这个函数。在笔记 [《Android Dynamic Linker 初始化流程总览》][2] 中我们整体分析了 `__linker_init()` 这个函数，在最终跳转到应用程序的真正入口 `main()` 之前，该函数执行了一些初始化工作，这些初始化工作分为两个阶段，本文重点分析第一个阶段，其实也就是 `__linker_init()` 这个函数` 本身。我们直接对着代码做注解。

这个函数前有一段重要的注释。解释了第一阶段的工作重点，就是在进入第二阶段之前要解决 linker 自身的 relocation 问题。有关这部分的技术背景分析我们在笔记 [《Android Dynamic Linker 初始化流程总览》][2] 中已经总结了，请参考。

```cpp
/*
 * This is the entry point for the linker, called from begin.S. This
 * method is responsible for fixing the linker's own relocations, and
 * then calling __linker_init_post_relocation().
 *
 * Because this method is called before the linker has fixed it's own
 * relocations, any attempt to reference an extern variable, extern
 * function, or other GOT reference will generate a segfault.
 */
extern "C" ElfW(Addr) __linker_init(void* raw_args) {
```

函数一开始是初始化 C runtime 环境对线程的支持。有关线程支持中最核心的内容涉及 TLS（Thread Local Storage），这个我思量需要另外成篇介绍，这里我们只要知道有关线程初始化部分不是那么简单，`__libc_init_main_thread_early()` 这个函数并不是全部。完整的线程初始化分成了三个函数先后在一个流程的三处不同地方调用，原因我这里暂不展开分析。这三个部分包括：

- 第一部分为 `__libc_init_main_thread_early()` 在这里先调用，目的是早早初始化 TLS
- 第二部分为 `__libc_init_main_thread_late()`, 这部分在 `__linker_init_post_relocation()` 被调用
- 第三部分为 `__libc_init_main_thread_final()`; 这部分在 `linker_main()` 中被调用

从函数的命名可以看出，这些函数属于 libc 的范畴，对于静态链接的程序来说，也有对线程环境的初始化以及这三部分的调用。所以动态链接这里只是复用了这些操作。我们在读 `__linker_init()` 函数中涉及线程初始化的内容时可以和 `__real_libc_init()` 这个函数进行对比。

- `__real_libc_init()` -> 静态链接可执行程序的初始化过程
- `__linker_init()` -> 动态链接可执行程序的初始化过程

注意在对 `temp_tcb` 进行内存置空时并没有使用标准的 string 类函数，譬如 `memset()` 等，这个原因和 ifunc 机制此时还没有初始化有关。所以 Android linker 自己提供了一个相对 general 但比较低效的实现 `linker_memclr()`。

```cpp
  // Initialize TLS early so system calls and errno work.
  KernelArgumentBlock args(raw_args);
  bionic_tcb temp_tcb __attribute__((uninitialized));
  linker_memclr(&temp_tcb, sizeof(temp_tcb));
  __libc_init_main_thread_early(args, &temp_tcb);
```

在通过系统调用 `execve()` 加载程序时，如果是动态链接程序，内核会将 dynamic linker（本质是一个 so）加载到内存，在跳转到 dynamic linker 程序的入口函数执行前，内核会将 dynamic linker 在内存中实际映射的基地址通过 stack 中的 AT_BASE 参数传递给我们，这种情况下 `linker_addr` 就是一个非零值。（TBD，好像没有总结过内核在跳转到用户态程序之前的环境准备问题，这个有机会再另外总结一篇。）

我们也可以直接运行 dynamic linker，对于这种情况，则 `linker_addr` 的值为 0，对于这种情况，则我们需要调用 `get_elf_base_from_phdr()` 自己计算 `linker_addr`。具体怎么计算不是我们关心的重点，大家可以自己看。

```cpp
  // When the linker is run by itself (rather than as an interpreter for
  // another program), AT_BASE is 0.
  ElfW(Addr) linker_addr = getauxval(AT_BASE);
  if (linker_addr == 0) {
    // The AT_PHDR and AT_PHNUM aux values describe this linker instance, so use
    // the phdr to find the linker's base address.
    ElfW(Addr) load_bias;
    get_elf_base_from_phdr(
      reinterpret_cast<ElfW(Phdr)*>(getauxval(AT_PHDR)), getauxval(AT_PHNUM),
      &linker_addr, &load_bias);
  }
```

此时拿到 dynamic linker 在内存中的基地址 `linker_addr`，基于 `linker_addr` 获取 linker 在内存中的 Ehdr 和 Phdr 结构的地址。
- Ehdr：ELF Header
- Phdr：Program Header Table

```cpp
  ElfW(Ehdr)* elf_hdr = reinterpret_cast<ElfW(Ehdr)*>(linker_addr);
  ElfW(Phdr)* phdr = reinterpret_cast<ElfW(Phdr)*>(linker_addr + elf_hdr->e_phoff);
```

这里解决 dynamic linker 的 ifunc, 具体参考 [《BIONIC 中对 IFUNC 的支持》][3]。

```cpp
  // string.h functions must not be used prior to calling the linker's ifunc resolvers.
  call_ifunc_resolvers();
```

下面是在为调用 `__linker_init_post_relocation()` 准备一个 `tmp_linker_so`。这也是 `__linker_init()` 函数中的一个很重要的一个动作。
 
`tmp_linker_so` 是一个 soinfo 对象。一个 soinfo 对象在 linker 里面是代表 shared object 的一个类实例，每个加载到内存的 so 库都会有一个 soinfo 对象表示，同一个动态库文件 dlopen 两次就会创建两个 soinfo 对象，并且动态库文件被映射到不同的逻辑内存地址上。

`tmp_linker_so` 所创建的 so 对象对应的就是 linker 自身，因为 linker 本质上也是一个 shared object，其 soname 是 "ld-android.so"。

在内存管理上，我们需要注意 `tmp_linker_so` 是在栈上构造的一个临时的 soinfo 对象，在第一阶段临时用一下，解决完 linker 自身的 relocation 问题后会将 `tmp_linker_so` 继续传入 `__linker_init_post_relocationsoinfo()` 继续初始化，此时会使用 `get_libdl_info()` 函数在堆上基于这个 `tmp_linker_so` 重新构造一个最终的 linker soinfo 对象，并将其加入一个叫做 `solist` 的 linked_list 上集中管理。所有 soinfo 对象都会放入 solist 列表中进行管理，而 linker 是 linked_list 上的第一个 soinfo 对象。我理解之所以要在 `__linker_init_post_relocation()` 中才能涉及堆操作，主要还是和第一阶段 "fixing the linker's own relocations" 有关，在 fixing 之前我们是不可以调用 malloc 等库函数的。

```cpp
  soinfo tmp_linker_so(nullptr, nullptr, nullptr, 0, 0);
```

下面是在对 stack 上创建的临时 linker soinfo 对象进行初始化。

`phdr_table_get_load_size()` 函数用于计算 program headers 中所有 PT_LOAD 的 entry 的长度之和，dlopen 加载 ELF 格式的动态库时，除了映射相应的头部数据，会将program headers 中所有 PT_LOAD 的 entry，也就是我们常说的 segment 分别 map 到内存（通过 `ElfReader::LoadSegments()` 方法）。

初始化最后通过 `set_linker_flag()` 标记自己为 linker，这个标记在后面很多地方用于区分 linker 和一般的 so 对象。

```cpp
  tmp_linker_so.base = linker_addr;
  tmp_linker_so.size = phdr_table_get_load_size(phdr, elf_hdr->e_phnum);
  tmp_linker_so.load_bias = get_elf_exec_load_bias(elf_hdr);
  tmp_linker_so.dynamic = nullptr;
  tmp_linker_so.phdr = phdr;
  tmp_linker_so.phnum = elf_hdr->e_phnum;
  tmp_linker_so.set_linker_flag();
```

这里注释中的所谓 Prelink 实际上就是我们常说的 relocation。也就是在完成上面的种种准备工作后，linker 终于开始 "fixing the linker's own relocations"。

调用 `soinfo::prelink_image()`, 这个函数的作用针对 so 文件解析出 `.dynamic`、符号表、字符串表、got、plt、hash 表等等数据结构的内存位置、大小和一些相关参数。如果失败则调用 `__linker_cannot_link()` 报错终止执行
   
调用 `soinfo::linker_image()` 函数对 linker 做重定位，如果失败则调用 `__linker_cannot_link()` 报错终止执行

```cpp
  // Prelink the linker so we can access linker globals.
  if (!tmp_linker_so.prelink_image()) __linker_cannot_link(args.argv[0]);
  if (!tmp_linker_so.link_image(SymbolLookupList(&tmp_linker_so), &tmp_linker_so, nullptr, nullptr)) __linker_cannot_link(args.argv[0]);
```

此时 linker 自身重定位的工作完成，此后 linker 可以正常访问所有全局的非静态变量。`__linker_init()` 的第一阶段完成，进入 `__linker_init_post_relocation()` 开始第二阶段的工作。

```cpp
  return __linker_init_post_relocation(args, tmp_linker_so);
}
```

[1]:./20221220-andorid-linker-entry.md
[2]:./20221222-android-dynamic-linker-overview.md
[3]:./20220623-ifunc-bionic.md
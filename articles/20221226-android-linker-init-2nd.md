![](./diagrams/android-riscv.png)

文章标题：**Android Dynamic Linker 初始化流程的第二阶段处理**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

Android 的 bionic 仓库中实现了 Android 系统的 Dynamic linker（bionic 中也简称 linker，本文中如果提及 linker 也指 dynamic linker）。

linker 的具体代码在 `<AOSP>/bionic/linker` 下。下文以 `<AOSP>` 指代 aosp 采用 repo sync 后的源码根目录路径。本文代码基于 commit 6f78f756a242c3cb82476d9e5e0c1f85faf51e08，涉及 ARCH 时以 riscv 为例。

在笔记 [《Android Dynamic Linker 的入口》][1]中我们知道当我们在 Android 上执行一个动态链接的应用程序时，操作系统会首先加载 dynamic linker 并跳转到 dynamic linker 的入口函数 `_start()` 处开始执行，这个函数实际上没干啥，直接再次调用 `__linker_init()` 这个函数。在笔记 [《Android Dynamic Linker 初始化流程总览》][2] 中我们整体分析了 `__linker_init()` 这个函数，在最终跳转到应用程序的真正入口 `main()` 之前，该函数执行了一些初始化工作，这些初始化工作分为两个阶段，第一个阶段总结在笔记 [《Android Dynamic Linker 初始化流程的第一阶段处理》][4]，而本文重点分析第二个阶段。

第二个阶段的入口是 `__linker_init_post_relocation()` 函数，我们对着代码来看一下它究竟做了些什么。

# `__linker_init_post_relocation()` 函数代码注解

```cpp
/*
 * This code is called after the linker has linked itself and fixed its own
 * GOT. It is safe to make references to externs and other non-local data at
 * this point. The compiler sometimes moves GOT references earlier in a
 * function, so avoid inlining this function (http://b/80503879).
 */
static ElfW(Addr) __attribute__((noinline))
__linker_init_post_relocation(KernelArgumentBlock& args, soinfo& tmp_linker_so) {
```

对 `__libc_init_main_thread_late()` 的调用是和 `__linker_init()` 中的 `__libc_init_main_thread_early()` 配合一起完成了对 libc 的 main thread 的初始化，具体解释需要放到 TLS 的部分去分析。
```cpp
  // Finish initializing the main thread.
  __libc_init_main_thread_late();
```

`tmp_linker_so` 是我们在 linker 初始化的第一阶段中在 stack 上创建的临时 soinfo 对象。这里我们继续利用这个临时对象调用 `soinfo::protect_relro()` 将 PT_GNU_RELRO 段指向的内存地址通过 mprotoct 函数设置为 PROT_READ。

```cpp
  // We didn't protect the linker's RELRO pages in link_image because we
  // couldn't make system calls on x86 at that point, but we can now...
  if (!tmp_linker_so.protect_relro()) __linker_cannot_link(args.argv[0]);

  // And we can set VMA name for the bss section now
  set_bss_vma_name(&tmp_linker_so);
```

通过调用 `__libc_init_globals()` 将 libc 的全局变量初始化完成后，heap 才真正可以使用。
```cpp
  // Initialize the linker's static libc's globals
  __libc_init_globals();
```

`soinfo::call_constructors()` 这个函数会递归遍历一个 so 以及其依赖的 so，并对这些 so 执行对应的 `init_func_` 函数和 `init_array_` 列表中的函数。 

`init_func_` 函数对应 ELF 文件中 dynamic section 中的 `DT_INIT` entry 指向的函数；`init_array_` 列表则是 dynamic section 中的 `DT_INIT_ARRAY` entry 包含的函数列表。`DT_INIT_ARRAY` 段中的函数列表是代码中通过 `__attribute__ ((constructor))` 前缀修饰的全局函数，以及针对 c++ 编译为一些全局变量生成的构造函数。

```cpp
  // Initialize the linker's own global variables
  tmp_linker_so.call_constructors();

  // Setting the linker soinfo's soname can allocate heap memory, so delay it until here.
  for (const ElfW(Dyn)* d = tmp_linker_so.dynamic; d->d_tag != DT_NULL; ++d) {
    if (d->d_tag == DT_SONAME) {
      tmp_linker_so.set_soname(tmp_linker_so.get_string(d->d_un.d_val));
    }
  }
```

在这里判断当前 linker 是否是直接被运行，如果是则分析参数并执行。判断的方法主要是看内核传递过来的 AT_ENTRY 的值。当 linker 是直接运行时 AT_ENTRY 的值就是 linker 的 `_start` 的值，否则是可执行程序的地址。

针对如果是直接运行 linker 则会根据命令行参数不同走三种情况：
- case 1：linker --list executable，类似执行 ldd 的效果
- case 2: linker --help 或者 linker, 打印帮助就直接退出了
- case 3: linker executable

注意这里的 `exe_to_load`, 当直接运行 linker 时， 这个变量会被赋值为上面 case 1 或者 case 2 中 executable 的 path string；对于直接运行可执行程序的，则这个变量保持为 NULL。`exe_to_load` 赋值后会被后面 `linker_main()` 使用。

```cpp
  // When the linker is run directly rather than acting as PT_INTERP, parse
  // arguments and determine the executable to load. When it's instead acting
  // as PT_INTERP, AT_ENTRY will refer to the loaded executable rather than the
  // linker's _start.
  const char* exe_to_load = nullptr;
  if (getauxval(AT_ENTRY) == reinterpret_cast<uintptr_t>(&_start)) {
    if (args.argc == 3 && !strcmp(args.argv[1], "--list")) {
      // We're being asked to behave like ldd(1).
      g_is_ldd = true;
      exe_to_load = args.argv[2];
    } else if (args.argc <= 1 || !strcmp(args.argv[1], "--help")) {
      async_safe_format_fd(STDOUT_FILENO,
         "Usage: %s [--list] PROGRAM [ARGS-FOR-PROGRAM...]\n"
         "       %s [--list] path.zip!/PROGRAM [ARGS-FOR-PROGRAM...]\n"
         "\n"
         "A helper program for linking dynamic executables. Typically, the kernel loads\n"
         "this program because it's the PT_INTERP of a dynamic executable.\n"
         "\n"
         "This program can also be run directly to load and run a dynamic executable. The\n"
         "executable can be inside a zip file if it's stored uncompressed and at a\n"
         "page-aligned offset.\n"
         "\n"
         "The --list option gives behavior equivalent to ldd(1) on other systems.\n",
         args.argv[0], args.argv[0]);
      _exit(EXIT_SUCCESS);
    } else {
      exe_to_load = args.argv[1];
      __libc_shared_globals()->initial_linker_arg_count = 1;
    }
  }
```

三种情况会走到这里：
- execve 一个动态链接程序时，linker 作为 interpreter 被内核调用，这也是 linker 的最常见的应用场景
- linker 被直接执行，而且是上面列出的直接执行条件下的 case 1 和 case 3。case 2 不会走到这里，打印帮助后就会退出。

走到这里再往下继续执行，后面的核心逻辑就是要准备执行 `linker_main()` 了。

```cpp
  // store argc/argv/envp to use them for calling constructors
  g_argc = args.argc - __libc_shared_globals()->initial_linker_arg_count;
  g_argv = args.argv + __libc_shared_globals()->initial_linker_arg_count;
  g_envp = args.envp;
  __libc_shared_globals()->init_progname = g_argv[0];
```

调用 `get_libdl_info()` 相当于在堆上拷贝 `tmp_linker_so` 构造了一个新的 soinfo，然后添加到 solist 和 sonext 链表中。所以加入 linked_list 的不是 `tmp_linker_so`, 而是这里的 solinker。
```cpp
  // Initialize static variables. Note that in order to
  // get correct libdl_info we need to call constructors
  // before get_libdl_info().
  sonext = solist = solinker = get_libdl_info(tmp_linker_so);
  g_default_namespace.add_soinfo(solinker);
```

调用 linker_main 时传入 args 以及 `exe_to_load`，即应用的名字。`linker_main()` 会返回可执行程序的地址，该地址会最终返回到最初的 `_start` 函数中。

详细的针对 `linker_main()` 函数的分析见下一章节。

```cpp
  ElfW(Addr) start_address = linker_main(args, exe_to_load);
```

如果只是执行类似 ldd 的功能，即上面提到的直接运行 linker 中的 case 1，那么就不需要再执行实际的可执行程序，这里就退出了

```cpp
  if (g_is_ldd) _exit(EXIT_SUCCESS);
```

在 log 中 注意这个打印，它标志着 linker 的初始化工作（包括所有的 load 和 link 工作）完成并准备跳转到可执行程序的 main 入口。
```cpp
  INFO("[ Jumping to _start (%p)... ]", reinterpret_cast<void*>(start_address));

  // Return the address that the calling assembly stub should jump to.
  return start_address;
}
```

# `linker_main()` 函数代码注解

这个函数相对比较长，我这里就只挑我感兴趣的部分注解，其他部分读者可以自行分析。

```cpp
static ElfW(Addr) linker_main(KernelArgumentBlock& args, const char* exe_to_load) {
  ProtectedDataGuard guard;

#if TIMING
  struct timeval t0, t1;
  gettimeofday(&t0, 0);
#endif

  // Sanitize the environment.
  __libc_init_AT_SECURE(args.envp);

  // Initialize system properties
  __system_properties_init(); // may use 'environ'

  // Initialize platform properties.
  platform_properties_init();

  // Register the debuggerd signal handler.
  linker_debuggerd_init();

  g_linker_logger.ResetState();

  // Get a few environment variables.
  const char* LD_DEBUG = getenv("LD_DEBUG");
  if (LD_DEBUG != nullptr) {
    g_ld_debug_verbosity = atoi(LD_DEBUG);
  }
```

前面主要是做一些初始化，初始化完成后，在 log 中打印一些基本环境的基本信息，特别地，如果看到 "[ Android dynamic linker (64-bit) ]" 说明程序进入 `linker_main()` 函数了。

```cpp
#if defined(__LP64__)
  INFO("[ Android dynamic linker (64-bit) ]");
#else
  INFO("[ Android dynamic linker (32-bit) ]");
#endif

  // These should have been sanitized by __libc_init_AT_SECURE, but the test
  // doesn't cost us anything.
  const char* ldpath_env = nullptr;
  const char* ldpreload_env = nullptr;
  if (!getauxval(AT_SECURE)) {
    ldpath_env = getenv("LD_LIBRARY_PATH");
    if (ldpath_env != nullptr) {
      INFO("[ LD_LIBRARY_PATH set to \"%s\" ]", ldpath_env);
    }
    ldpreload_env = getenv("LD_PRELOAD");
    if (ldpreload_env != nullptr) {
      INFO("[ LD_PRELOAD set to \"%s\" ]", ldpreload_env);
    }
  }
```

如果是我们前面提到的直接运行 linker（case 1 和 case 3），在这种情况下我们需要自己加载可执行程序。而大部分情况下，当 linker 作为 PT_INTERP 被调用时，内核已经在执行 linker 前加载过可执行程序，不需要在这里重复加载，注意 exe_to_load 默认初始化为 nullptr 就是达到了这个效果。

无论是否需要加载可执行程序，都会在这里得到一个 ExecutableInfo 类型的 `exe_info`。然后代码会打印出 "[ Linking executable ...... ]", log 中看到这个说明应用程序已经加载成功，

注意此时加载程序，并还没有同时加载其依赖的（NEEDED）其他 so，更还没有为 so 处理 relocation 问题。相关操作放在后面。

```cpp
  const ExecutableInfo exe_info = exe_to_load ? load_executable(exe_to_load) :
                                                get_executable_info();

  INFO("[ Linking executable \"%s\" ]", exe_info.path.c_str());
```

为可执行程序生成 soinfo，可执行程序也当成一个 so 对待， 调用 `soinfo_alloc()` 生成一个 soinfo 对象。并将改对象保存在 somain 这个全局静态指针中, 然后加入 link_map 作为表头, 看注释应该是 the one after libdl_info

```cpp
  // Initialize the main exe's soinfo.
  soinfo* si = soinfo_alloc(&g_default_namespace,
                            exe_info.path.c_str(), &exe_info.file_stat,
                            0, RTLD_GLOBAL);
  somain = si;
  si->phdr = exe_info.phdr;
  si->phnum = exe_info.phdr_count;
  get_elf_base_from_phdr(si->phdr, si->phnum, &si->base, &si->load_bias);
  si->size = phdr_table_get_load_size(si->phdr, si->phnum);
  si->dynamic = nullptr;
  si->set_main_executable();
  init_link_map_head(*si);

  set_bss_vma_name(si);

  // Use the executable's PT_INTERP string as the solinker filename in the
  // dynamic linker's module list. gdb reads both PT_INTERP and the module list,
  // and if the paths for the linker are different, gdb will report that the
  // PT_INTERP linker path was unloaded once the module list is initialized.
  // There are three situations to handle:
  //  - the APEX linker (/system/bin/linker[64] -> /apex/.../linker[64])
  //  - the ASAN linker (/system/bin/linker_asan[64] -> /apex/.../linker[64])
  //  - the bootstrap linker (/system/bin/bootstrap/linker[64])
  const char *interp = phdr_table_get_interpreter_name(somain->phdr, somain->phnum,
                                                       somain->load_bias);
  if (interp == nullptr) {
    // This case can happen if the linker attempts to execute itself
    // (e.g. "linker64 /system/bin/linker64").
    interp = kFallbackLinkerPath;
  }
  solinker->set_realpath(interp);
  init_link_map_head(*solinker);

#if defined(__aarch64__)
  if (exe_to_load == nullptr) {
    // Kernel does not add PROT_BTI to executable pages of the loaded ELF.
    // Apply appropriate protections here if it is needed.
    auto note_gnu_property = GnuPropertySection(somain);
    if (note_gnu_property.IsBTICompatible() &&
        (phdr_table_protect_segments(somain->phdr, somain->phnum, somain->load_bias,
                                     &note_gnu_property) < 0)) {
      __linker_error("error: can't protect segments for \"%s\": %s", exe_info.path.c_str(),
                     strerror(errno));
    }
  }

  __libc_init_mte(somain->phdr, somain->phnum, somain->load_bias, args.argv);
#endif

  // Register the main executable and the linker upfront to have
  // gdb aware of them before loading the rest of the dependency
  // tree.
  //
  // gdb expects the linker to be in the debug shared object list.
  // Without this, gdb has trouble locating the linker's ".text"
  // and ".plt" sections. Gdb could also potentially use this to
  // relocate the offset of our exported 'rtld_db_dlactivity' symbol.
  //
  insert_link_map_into_debug_map(&si->link_map_head);
  insert_link_map_into_debug_map(&solinker->link_map_head);

  add_vdso();

  ElfW(Ehdr)* elf_hdr = reinterpret_cast<ElfW(Ehdr)*>(si->base);

  // We haven't supported non-PIE since Lollipop for security reasons.
  if (elf_hdr->e_type != ET_DYN) {
    // We don't use async_safe_fatal here because we don't want a tombstone:
    // even after several years we still find ourselves on app compatibility
    // investigations because some app's trying to launch an executable that
    // hasn't worked in at least three years, and we've "helpfully" dropped a
    // tombstone for them. The tombstone never provided any detail relevant to
    // fixing the problem anyway, and the utility of drawing extra attention
    // to the problem is non-existent at this late date.
    async_safe_format_fd(STDERR_FILENO,
                         "\"%s\": error: Android 5.0 and later only support "
                         "position-independent executables (-fPIE).\n",
                         g_argv[0]);
    _exit(EXIT_FAILURE);
  }
```

处理 linker 的 namespace 问题，这个专题比较大，考虑另外单独写一篇分析 TBD。

`init_default_namespaces()` 会初始化缺省的 linker namespace，log 中会打印 "`INFO("[ Reading linker config ...... ]"

```cpp
  // Use LD_LIBRARY_PATH and LD_PRELOAD (but only if we aren't setuid/setgid).
  parse_LD_LIBRARY_PATH(ldpath_env);
  parse_LD_PRELOAD(ldpreload_env);

  std::vector<android_namespace_t*> namespaces = init_default_namespaces(exe_info.path.c_str());
```

对 exe 的 soinfo 执行 `prelink_image()`, 这个函数通过分析 `.dynamic` section 的每个 entry 提取出我们关心的信息，譬如分析 NEEDED entry 获得 exec 依赖的 so。
```cpp
  if (!si->prelink_image()) __linker_cannot_link(g_argv[0]);

  // add somain to global group
  si->set_dt_flags_1(si->get_dt_flags_1() | DF_1_GLOBAL);
  // ... and add it to all other linked namespaces
  for (auto linked_ns : namespaces) {
    if (linked_ns != &g_default_namespace) {
      linked_ns->add_soinfo(somain);
      somain->add_secondary_namespace(linked_ns);
    }
  }

  linker_setup_exe_static_tls(g_argv[0]);
```

计算依赖的 so 的列表并把这些 so 的名字记录在 `needed_library_name_list` 这个数组中。`needed_libraries_count` 记录的是 `needed_library_name_list` 数组的大小。

如果 `needed_libraries_count` 大于 0，说明程序依赖于至少一个 so，则通过调用 `find_libraries()` 这个函数将这些依赖的 so 加载进来并完成 relocation；否则如果 `needed_libraries_count` = 0，则说明程序没有依赖的 so，所以只对 somain，也就是应用程序自身（对于 linker ，exec 和 so 一样都视为 module） 执行 relocation。

`find_libraries()` 这个函数很重要，里面的工作是 linker 的核心，我会单独再为它写一篇笔记 TBD。

```cpp
  // Load ld_preloads and dependencies.
  std::vector<const char*> needed_library_name_list;
  size_t ld_preloads_count = 0;

  for (const auto& ld_preload_name : g_ld_preload_names) {
    needed_library_name_list.push_back(ld_preload_name.c_str());
    ++ld_preloads_count;
  }

  for_each_dt_needed(si, [&](const char* name) {
    needed_library_name_list.push_back(name);
  });

  const char** needed_library_names = &needed_library_name_list[0];
  size_t needed_libraries_count = needed_library_name_list.size();

  if (needed_libraries_count > 0 &&
      !find_libraries(&g_default_namespace,
                      si,
                      needed_library_names,
                      needed_libraries_count,
                      nullptr,
                      &g_ld_preloads,
                      ld_preloads_count,
                      RTLD_GLOBAL,
                      nullptr,
                      true /* add_as_children */,
                      &namespaces)) {
    __linker_cannot_link(g_argv[0]);
  } else if (needed_libraries_count == 0) {
    if (!si->link_image(SymbolLookupList(si), si, nullptr, nullptr)) {
      __linker_cannot_link(g_argv[0]);
    }
    si->increment_ref_count();
  }
```

线程初始化的最后收尾工作在这里完成，具体分析在 TLS 的笔记中再总结。

```cpp
  linker_finalize_static_tls();
  __libc_init_main_thread_final();
```

CFI 处理。期间会打印诸如：
```
SEARCH __cfi_check in /system/bin/bootstrap/linker64@0x0 (gnu)
NOT FOUND __cfi_check in /system/bin/bootstrap/linker64@0x0
```

```cpp
  if (!get_cfi_shadow()->InitialLinkDone(solist)) __linker_cannot_link(g_argv[0]);
```

对 exec 处理 pre_init 数组和 init 数组
```cpp
  si->call_pre_init_constructors();
  si->call_constructors();
```

```cpp
#if TIMING
  gettimeofday(&t1, nullptr);
  PRINT("LINKER TIME: %s: %d microseconds", g_argv[0],
        static_cast<int>(((static_cast<long long>(t1.tv_sec) * 1000000LL) +
                          static_cast<long long>(t1.tv_usec)) -
                         ((static_cast<long long>(t0.tv_sec) * 1000000LL) +
                          static_cast<long long>(t0.tv_usec))));
#endif
#if STATS
  print_linker_stats();
#endif
#if TIMING || STATS
  fflush(stdout);
#endif

  // We are about to hand control over to the executable loaded.  We don't want
  // to leave dirty pages behind unnecessarily.
  purge_unused_memory();
```

程序执行到这里，可执行文件的加载基本完成，从可执行程序的 `entry_point` 中读出入口地址，返回给 `__linker_init_post_relocation()` 函数

log 会打印形如 "[ Ready to execute ...... @ .... ]"

```cpp
  ElfW(Addr) entry = exe_info.entry_point;
  TRACE("[ Ready to execute \"%s\" @ %p ]", si->get_realpath(), reinterpret_cast<void*>(entry));
  return entry;
}
```



[1]:./20221220-andorid-linker-entry.md
[2]:./20221222-android-dynamic-linker-overview.md
[3]:./20220623-ifunc-bionic.md
[4]:./20221226-android-linker-init-1st.md
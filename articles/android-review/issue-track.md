基于 <https://github.com/google/android-riscv64/issues>, 这里的 issue 由 Google team 整理，整合了原先的
[RISC-V Android SIG Gap Analysis](https://docs.google.com/spreadsheets/d/1HifwLJCBeLxgtXo-D1O1POlsBlybRokCmAj3yljq9NM/edit#gid=0)

我根据我的理解按照功能模块分了一下类，看看我们可以参与哪些：

# LLVM/Clang/Rust：

可以用 llvm/toolchain 标签进行过滤。如果要介入需要在 llvm/clang 上有深厚的积累。

- **CLOSED**[#82 clang driver changes that must happen before even an initial NDK][82]: Duplicated with [81][81]
- [#81 compiler must-haves for RISCV-android][81]
- **CLOSED**[#77 [Investigation] Verify if binary is built with SCS reserved register.][77]
- **CLOSED**[#76 Enable -msave-restore for binary size][76] 
  
  `-msave-restore` 的作用是利用通用的公共函数实现 prologue 和 epilouge。好处是可以优化 code size，但坏处是会带来性能上的损失。可以参考一篇介绍 [淺談 RISCV 裡的 save-restore call](https://hackmd.io/@5iODz6_2TQWOK-sMRViU2Q/S1BCrnprv)。目前的讨论结果是现在针对 riscv，优化 code size 远没有优化执行 performace 来的急迫，所以这被认为是一个低优先级的问题，暂时不研究。
  
  TBD，issue 讨论中 asb 提到了 [RVA23 Profile](https://github.com/riscv/riscv-profiles/blob/main/rva23-profile.adoc): `RVA23 is intended to be a major release of the RISC-V Application Processor Profiles.` 上面列举了一款 64-bit 的 application processor 上至少必须支持的 RISC-V 的扩展标准，值得继续研究。

- **CLOSED**[#74 investigate if llvm requires any changes to make LR/SC sequence sequentially consistent][74]: Duplicated with [73][73].
- [#73 clang compiler should emit atomic sequences that are compatible with hboehm's suggested new instructions][73]
- [#72 reserve a register in the clang compiler driver's defaults for android [llvm]][72]
- [#71 once V and Zb* are working in cuttlefish, add them to the clang driver's defaults for android [llvm]][71]
- [#69 llvm function multi-versioning][69]
- [#68 Binary analysis of aosp to compare Aarch64 vs RISC-V][68]
- [#67 Comparative analysis of compiler statistics between Aarch64 and RISC-V][67]
- [#64 crash with RISC-V scalable vectorization and kernel address sanitizer #61096][64]
- [#63 need support '.option arch' directive (https://reviews.llvm.org/D123515) to enable linux 6.3 Zbb optimizations?][63]
- [#62 Enable -msave-restore at -Oz][62]
- [#61 Fix ABI and mcpu/march for LTO][61]
- [#58 Fix platform:Android bugs in llvm-project][58]
- **CLOSED**[#57 Fix RISC-V bugs in llvm upstream][57]

  这个问题有关 emulated TLS 在 RISC-V 上的实现。目前看上去 LLVM 主线对 RISC-V 已经支持了 emulated TLS
  
  TBD：需要及时总结一下 TLS 在 RISC-V 上实现，以及 emulated TLS 的相关内容。

- [#51 ship libomp.a and libomp.so][51] in runtime_ndk_xxx for prebuilt clang
- [#50 ship libFuzzer.a][50] in runtime_ndk_xxx for prebuilt clang
- **CLOSED**[#47 llvm: fix emutls for risc-v (probably not?)][47]: 仍然是 emutls 的问题，参考 [57][57]
- [#46 llvm: function alignment][46]
- [#23 Investigate the current state of Auto-vectorization for RISC-V targets][23]
- [#22 llvm: make LTO work ][22]
- [#20 lld: investigate state of linker relaxation][20]
- **CLOSED**[#19 llvm: make sure "[RISCV] Allow mismatched SmallDataLimit and use Min for conflicting values" gets merged][19]:已解决
- [#18 llvm: missing libunwind support][18]
- [#16 llvm: prebuilts for hwasan support][16]
- [#9 clang: check that the global clang drivers riscv64 default flags make sense for Android][9]

# 内核部分：
- **CLOSED** [#79 kernel: CONFIG_SMP][79]
- [#75 kernel: missing hardware breakpoint support][75]
- [#56 kernel: software CFI][56]
- [#55 kernel: software shadow call stack][55]
- [#54 kernel: address space layout randomization][54]
- [#53 kernel: crypto optimization][53]
- [#27 kernel: HAVE_EFFICIENT_UNALIGNED_ACCESS][27]
- [#1 `system/core/init/`: more ASLR bits when we have Kconfig support for more][1]


# ART

基本功能 T-head 已经实现，但未上传 patch，我们能做的可能是基于它的一些优化，但问题是 ART 我们以前未接触过，而且 ART 比较复杂，学习曲线会较长。

# bionic

主要修改 T-head 已经提交并且合并完毕。我们可以参与的工作包括优化，以及执行 unit-test 并解决可能的 bug。

- **CLOSED** [#52 security: software shadow call stack support][52]

  软件实现 Shadow Call Stack。目前 bionic 中的实现是模仿 AARCH64 的，但后面有可能要切换为采用 gp。

  TBD：需要总结一下 SCS

- **CLOSED**[#12 llvm: should sqrt/sqrtf and the lrint/lrintf/llrint/llrintf family be builtins?][12]: 已解决
  
  TBD：可能需要再看一下 bionic 部分的修改并总结一下。

- **CLOSED**[#11 bionic: switch over last builtins in libm once new clang lands][11]: 已解决

  TBD：可能需要再看一下 bionic 部分的修改并总结一下。

- [#7 bionic: assembler versions of the mem* and str* functions using the V extension][7]
- **CLOSED**[#6 bionic: assembler versions of the __mem*_chk functions?][6]: 已解决

  TBD：可能需要再看一下 bionic 部分的修改并总结一下。

- [#5 `bionic/tests/sys_ptrace_test.cpp`: add an instruction that writes more than 64 bits][5]
- [#4 bionic: should we have vdso support for cache flushing and-or `<sys/cacheflush.h>` (probably not)][4]
- [#3 bionic: implement TLSDESC (not yet in psabi)][3]

# 库 Optimization

优先级不高，但以前没有做过，而且如何测试是个问题。

- [#66 external/libaom: optimization][66]
- [#59 frameworks/av: optimization][59]
- [#49 external/zlib optimization][49]
- **CLOSED**[#43 external/lzma: optimization (probably not?)][43]: 无需优化，cancelled
- [#42 external/XNNPACK: optimization][42]
- [#41 external/renderscript-intrinsics-replacement-toolkit/: optimization][41]
- [#40 external/freetype: optimization (?)][40]
- [#39 external/skia: optimization][39]
- [#38 external/libyuv: optimization][38]
- [#37 external/libpng: optimization][37]
- [#36 external/boringssl: optimization][36]
- [#35 external/libjpeg-turbo: optimization][35]
- [#34 external/libmpeg2: optimization][34]
- [#33 external/libhevc/: optimization][33]
- [#32 external/libavc/: optimization][32]
- [#31 external/libopus/: do we need optimization? (probably not?)][31]
- [#30 external/libopus/][30]
- [#29 external/flac/: need V optimization][29]
- [#17 external/libvpx: missing optimized assembler][17]
- [#13 external/aac: inline assembler (clz, saturating arithmetic)][13]
- [#2 external/scudo/: optimize CRC32 when we have useful instructions][2]

# 支持 Studio

- [Studio: gradle support][28]
- [Studio: do we need goldfish too?][26]
- [lldb: does it work sufficiently well for Studio?][21]

# Specification 相关

- [Fix default usage of reserved register][78]
- [Investigate the status of SLP vectorizer][60]
- [security: enable software CFI][45]
- [hardware: better atomics][44]
- [security: hardware cfi ("landing pads") support][15]
- [security: hardware shadow call stack support][14]
- [whats the ifunc story, hwcap.h][8]


# 其他(未分类)

- [#70 CTS test to ensure that core features are homogenous][70]
- **CLOSED**[#65 build: vNDK not packaged for RISC-V builds][65] 已经解决，具体看 comments。

  TBD：一些涉及 ld.config.txt 的处理还要看看，这个 topic 没有仔细总结过。

- [#48 simpleperf][48]
- **CLOSED**[#25 cuttlefish: get riscv64 cuttlefish up and running][25]:已经可以运行 cf slim。还缺 V 和 Zb* 扩展支持，但我觉得如果需要跟踪可以另外开一个 issue track。而且对这些扩展的支持是不是输入底层 QEMU 的事情？
- [#24 renderscript: go from deprecation to removal][24] 历史代码清理
- [#10 bazel: microdroid will need bazel support for riscv64][10]

[82]:https://github.com/google/android-riscv64/issues/82
[81]:https://github.com/google/android-riscv64/issues/81
[79]:https://github.com/google/android-riscv64/issues/79
[78]:https://github.com/google/android-riscv64/issues/78
[77]:https://github.com/google/android-riscv64/issues/77
[76]:https://github.com/google/android-riscv64/issues/76
[75]:https://github.com/google/android-riscv64/issues/75
[74]:https://github.com/google/android-riscv64/issues/74
[73]:https://github.com/google/android-riscv64/issues/73
[72]:https://github.com/google/android-riscv64/issues/72
[71]:https://github.com/google/android-riscv64/issues/71
[70]:https://github.com/google/android-riscv64/issues/70
[69]:https://github.com/google/android-riscv64/issues/69
[68]:https://github.com/google/android-riscv64/issues/68
[67]:https://github.com/google/android-riscv64/issues/67
[66]:https://github.com/google/android-riscv64/issues/66
[65]:https://github.com/google/android-riscv64/issues/65
[64]:https://github.com/google/android-riscv64/issues/64
[63]:https://github.com/google/android-riscv64/issues/63
[62]:https://github.com/google/android-riscv64/issues/62
[61]:https://github.com/google/android-riscv64/issues/61
[60]:https://github.com/google/android-riscv64/issues/60
[59]:https://github.com/google/android-riscv64/issues/59
[58]:https://github.com/google/android-riscv64/issues/58
[57]:https://github.com/google/android-riscv64/issues/57
[56]:https://github.com/google/android-riscv64/issues/56
[55]:https://github.com/google/android-riscv64/issues/55
[54]:https://github.com/google/android-riscv64/issues/54
[53]:https://github.com/google/android-riscv64/issues/53
[52]:https://github.com/google/android-riscv64/issues/52
[51]:https://github.com/google/android-riscv64/issues/51
[50]:https://github.com/google/android-riscv64/issues/50
[49]:https://github.com/google/android-riscv64/issues/49
[48]:https://github.com/google/android-riscv64/issues/48
[47]:https://github.com/google/android-riscv64/issues/47
[46]:https://github.com/google/android-riscv64/issues/46
[45]:https://github.com/google/android-riscv64/issues/45
[44]:https://github.com/google/android-riscv64/issues/44
[43]:https://github.com/google/android-riscv64/issues/43
[42]:https://github.com/google/android-riscv64/issues/42
[41]:https://github.com/google/android-riscv64/issues/41
[40]:https://github.com/google/android-riscv64/issues/40
[39]:https://github.com/google/android-riscv64/issues/39
[38]:https://github.com/google/android-riscv64/issues/38
[37]:https://github.com/google/android-riscv64/issues/37
[36]:https://github.com/google/android-riscv64/issues/36
[35]:https://github.com/google/android-riscv64/issues/35
[34]:https://github.com/google/android-riscv64/issues/34
[33]:https://github.com/google/android-riscv64/issues/33
[32]:https://github.com/google/android-riscv64/issues/32
[31]:https://github.com/google/android-riscv64/issues/31
[30]:https://github.com/google/android-riscv64/issues/30
[29]:https://github.com/google/android-riscv64/issues/29
[28]:https://github.com/google/android-riscv64/issues/28
[27]:https://github.com/google/android-riscv64/issues/27
[26]:https://github.com/google/android-riscv64/issues/26
[25]:https://github.com/google/android-riscv64/issues/25
[24]:https://github.com/google/android-riscv64/issues/24
[23]:https://github.com/google/android-riscv64/issues/23
[22]:https://github.com/google/android-riscv64/issues/22
[21]:https://github.com/google/android-riscv64/issues/21
[20]:https://github.com/google/android-riscv64/issues/20
[19]:https://github.com/google/android-riscv64/issues/19
[18]:https://github.com/google/android-riscv64/issues/18
[17]:https://github.com/google/android-riscv64/issues/17
[16]:https://github.com/google/android-riscv64/issues/16
[15]:https://github.com/google/android-riscv64/issues/15
[14]:https://github.com/google/android-riscv64/issues/14
[13]:https://github.com/google/android-riscv64/issues/13
[12]:https://github.com/google/android-riscv64/issues/12
[11]:https://github.com/google/android-riscv64/issues/11
[10]:https://github.com/google/android-riscv64/issues/10
[9]:https://github.com/google/android-riscv64/issues/9
[8]:https://github.com/google/android-riscv64/issues/8
[7]:https://github.com/google/android-riscv64/issues/7
[6]:https://github.com/google/android-riscv64/issues/5
[5]:https://github.com/google/android-riscv64/issues/5
[4]:https://github.com/google/android-riscv64/issues/4
[3]:https://github.com/google/android-riscv64/issues/3
[2]:https://github.com/google/android-riscv64/issues/2
[1]:https://github.com/google/android-riscv64/issues/1
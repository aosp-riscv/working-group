基于 <https://github.com/google/android-riscv64/issues>, 这里的 issue 由 Google team 整理，整合了原先的
[RISC-V Android SIG Gap Analysis](https://docs.google.com/spreadsheets/d/1HifwLJCBeLxgtXo-D1O1POlsBlybRokCmAj3yljq9NM/edit#gid=0)

我根据我的理解按照功能模块分了一下类，看看我们可以参与哪些：

# LLVM/Clang/Rust：

可以用 llvm/toolchain 标签进行过滤。如果要介入需要在 llvm/clang 上有深厚的积累。

- [compiler must-haves for RISCV-android][81]
- [clang compiler should emit atomic sequences that are compatible with hboehm's suggested new instructions][73]
- [reserve a register in the clang compiler driver's defaults for android [llvm]][72]
- [once V and Zb* are working in cuttlefish, add them to the clang driver's defaults for android [llvm]][71]
- [llvm function multi-versioning][69]
- [Binary analysis of aosp to compare Aarch64 vs RISC-V][68]
- [Comparative analysis of compiler statistics between Aarch64 and RISC-V][67]
- [crash with RISC-V scalable vectorization and kernel address sanitizer #61096][64]
- [need support '.option arch' directive (https://reviews.llvm.org/D123515) to enable linux 6.3 Zbb optimizations?][63]
- [Enable -msave-restore at -Oz][62]
- [Fix ABI and mcpu/march for LTO][61]
- [Fix platform:Android bugs in llvm-project][58]
- [Fix RISC-V bugs in llvm upstream][57]
- [ship libomp.a and libomp.so][51] in runtime_ndk_xxx for prebuilt clang
- [ship libFuzzer.a][50] in runtime_ndk_xxx for prebuilt clang
- [llvm: function alignment][46]
- [Investigate the current state of Auto-vectorization for RISC-V targets][23]
- [llvm: make LTO work ][22]
- [lld: investigate state of linker relaxation][20]
- [llvm: missing libunwind support][18]
- [llvm: prebuilts for hwasan support][16]
- [clang: check that the global clang driver's riscv64 default flags make sense for Android][9]

# 内核部分：
- [kernel: missing hardware breakpoint support][75]
- [kernel: software CFI][56]
- [kernel: software shadow call stack][55]
- [kernel: address space layout randomization][54]
- [kernel: crypto optimization][53]
- [kernel: HAVE_EFFICIENT_UNALIGNED_ACCESS][27]
- [system/core/init/: more ASLR bits when we have Kconfig support for more][1]


# ART

基本功能 T-head 已经实现，但未上传 patch，我们能做的可能是基于它的一些优化，但问题是 ART 我们以前未接触过，而且 ART 比较复杂，学习曲线会较长。

# bionic

主要修改 T-head 已经提交并且合并完毕。我们可以参与的工作包括优化，以及执行 unit-test 并解决可能的 bug。

- [bionic: switch over last builtins in libm once new clang lands][11]
- [bionic/: assembler versions of the mem* and str* functions using the V extension][7]
- [bionic/tests/sys_ptrace_test.cpp: add an instruction that writes more than 64 bits][5]
- [bionic/: should we have vdso support for cache flushing and/or <sys/cacheflush.h>? (probably not?)][4]
- [bionic/: implement TLSDESC (not yet in psabi)][3]

# Emulator（cuttlefilsh）：

对 riscv 的支持 Google 那有人在做，最新的消息是还没有完成，目前只能启动到 uboot，无法进入 kernel。具体还要研究，而且对于我们来说 cuttlefish 以前没有看过，需要一段时间学习。

- [cuttlefish: get riscv64 cuttlefish up and running][25]


# 库 Optimization

优先级不高（6），但以前没有做过，而且如何测试是个问题。

- [external/libaom: optimization][66]
- [frameworks/av: optimization][59]
- [external/zlib optimization][49]
- [external/lzma: optimization (probably not?)][43]
- [external/XNNPACK: optimization][42]
- [external/renderscript-intrinsics-replacement-toolkit/: optimization][41]
- [external/freetype: optimization (?)][40]
- [external/skia: optimization][39]
- [external/libyuv: optimization][38]
- [external/libpng: optimization][37]
- [external/boringssl: optimization][36]
- [external/libjpeg-turbo: optimization][35]
- [external/libmpeg2: optimization][34]
- [external/libhevc/: optimization][33]
- [external/libavc/: optimization][32]
- [external/libopus/: do we need optimization? (probably not?)][31]
- [external/libopus/][30]
- [external/flac/: need V optimization][29]
- [external/libvpx: missing optimized assembler][17]
- [external/aac: inline assembler (clz, saturating arithmetic)][13]
- [external/scudo/: optimize CRC32 when we have useful instructions][2]

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
- [what's the ifunc story? hwcap.h][8]


# 其他(未分类)

- [CTS test to ensure that core features are homogenous][70]
- [simpleperf][48]
- [renderscript: go from deprecation to removal][24] 历史代码清理
- [bazel: microdroid will need bazel support for riscv64][10]


[81]:https://github.com/google/android-riscv64/issues/81
[78]:https://github.com/google/android-riscv64/issues/78
[75]:https://github.com/google/android-riscv64/issues/75
[73]:https://github.com/google/android-riscv64/issues/73
[72]:https://github.com/google/android-riscv64/issues/72
[71]:https://github.com/google/android-riscv64/issues/71
[70]:https://github.com/google/android-riscv64/issues/70
[69]:https://github.com/google/android-riscv64/issues/69
[68]:https://github.com/google/android-riscv64/issues/68
[67]:https://github.com/google/android-riscv64/issues/67
[66]:https://github.com/google/android-riscv64/issues/66
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

[51]:https://github.com/google/android-riscv64/issues/51
[50]:https://github.com/google/android-riscv64/issues/50
[49]:https://github.com/google/android-riscv64/issues/49
[48]:https://github.com/google/android-riscv64/issues/48

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

[17]:https://github.com/google/android-riscv64/issues/17
[16]:https://github.com/google/android-riscv64/issues/16
[15]:https://github.com/google/android-riscv64/issues/15
[14]:https://github.com/google/android-riscv64/issues/14
[13]:https://github.com/google/android-riscv64/issues/13

[11]:https://github.com/google/android-riscv64/issues/11
[10]:https://github.com/google/android-riscv64/issues/10
[9] :https://github.com/google/android-riscv64/issues/9
[8] :https://github.com/google/android-riscv64/issues/8
[7] :https://github.com/google/android-riscv64/issues/7

[5] :https://github.com/google/android-riscv64/issues/5
[4] :https://github.com/google/android-riscv64/issues/4
[3] :https://github.com/google/android-riscv64/issues/3
[2] :https://github.com/google/android-riscv64/issues/2
[1] :https://github.com/google/android-riscv64/issues/1
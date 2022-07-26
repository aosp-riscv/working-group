Project(aosp-riscv) progress log (time in reverse order).

- 2022-07-30 Statue update

  - RVI upstream: The synchronization work between aosp-riscv (<https://github.com/aosp-riscv>) and RVI upstream (<https://github.com/riscv-android-src>) is almost complete. We will not add new features to aosp-riscv, and the development work later will move to RVI upstream (https://github.com/riscv-android-src).
    - updated qemu for riscv: https://github.com/riscv-android-src/platform-prebuilts-android-emulator/pull/2
    - updated android 12 guide: https://github.com/riscv-android-src/riscv-android/pull/5
    - Updated the android12.md for cts: https://github.com/riscv-android-src/riscv-android/pull/6
    - rollback commented code: https://github.com/riscv-android-src/platform-bionic/pull/34
    - revert the changes: https://github.com/riscv-android-src/platform-frameworks-av/pull/1
    - remove build options for aosp 10: https://github.com/riscv-android-src/platform-build-soong/pull/6
    - remove.sdk_rv: https://github.com/riscv-android-src/platform-build/pull/4
    - revert.mainline_system_x86: https://github.com/riscv-android-src/platform-build/pull/5
    - remove.sdk_phone_riscv64: https://github.com/riscv-android-src/platform-build/pull/6
    - add -mno-relax when building libc++: https://github.com/riscv-android-src/toolchain-llvm_android/pull/3
  - bionic unit test ( on emulator) status update:
    - Round issue for math lib: https://gitee.com/aosp-riscv/working-group/issues/I5BV63, opened LLVM/compiler-rt PR: https://reviews.llvm.org/D128240, rework after review and waiting for reviewer accept.
    - Signal Stack unwinding failure: https://gitee.com/aosp-riscv/working-group/issues/I5D6NY, it is found that the build manifest of llvm-project does not pick the latest bionic and ndk, submitted an issue for discussion: https://github.com/riscv-android-src/toolchain-llvm-project/issues/5
    - "-nan" sprintf error: https://gitee.com/aosp-riscv/working-group/issues/I5CKA4, submitted a GNU toolchain issue: https://github.com/riscv-collab/riscv-gnu-toolchain/issues/1092, waiting for feedback
  - Articles:
    - RenderScript poriting analysis for AOSP RISC-V: https://gitee.com/aosp-riscv/working-group/blob/master/articles/20220509-renderscipt-adaptation-analysis-in-android12-riscv64-porting.md (in chinese)
    - Setup CTS env:  https://gitee.com/aosp-riscv/working-group/blob/master/articles/20220705-build-the-cts.md
    - Call Stack (RISC-V): https://zhuanlan.zhihu.com/p/542845861
    - Stack Unwinding - Overview: https://zhuanlan.zhihu.com/p/543823849
    - Stack Unwinding with Frame Pointer: https://zhuanlan.zhihu.com/p/543825539
    - Stack Unwinding with CFI: https://zhuanlan.zhihu.com/p/546207071
    - GCC cross toolchain for building RISC-V from source: https://zhuanlan.zhihu.com/p/544827596

- 2022-06-30 Statue update

  - RVI upstream:
    - roll-back temp changes: https://github.com/riscv-android-src/platform-bionic/pull/27
    - redefine ucontext: https://github.com/riscv-android-src/platform-bionic/pull/28
    - fixed benchmark for link reloc: https://github.com/riscv-android-src/platform-bionic/pull/29
    - Fix unistd.exec_argv0_null for new kernels.: https://github.com/riscv-android-src/platform-bionic/pull/32
    - use clang integrated as: https://github.com/riscv-android-src/platform-build-soong/pull/4
    - add no-relax: https://github.com/riscv-android-src/platform-build-soong/pull/5
    - updated android 12 download steps: https://github.com/riscv-android-src/riscv-android/pull/4
  - aosp-riscv development and bugfix:
    - added build of run-as: https://gitee.com/aosp-riscv/test-riscv/pulls/22
    - update env and add 8.log: https://gitee.com/aosp-riscv/test-riscv/pulls/21
    - added 9.log: https://gitee.com/aosp-riscv/test-riscv/pulls/23
    - Investigate round issue for math lib：
      - Root-Analysis: https://gitee.com/aosp-riscv/working-group/issues/I5BV63
      - Raised LLVM/compiler-rt PR: https://reviews.llvm.org/D128240 ，waiting for review.
    - Signal Stack unwinding failure：
      - Root-Analysis: https://gitee.com/aosp-riscv/working-group/issues/I5D6NY, in investigating
    - "-nan" sprintf error
      - Root-Analysis: https://gitee.com/aosp-riscv/working-group/issues/I5CKA4
      - submit a GNU toolchain issue: https://github.com/riscv-collab/riscv-gnu-toolchain/issues/1092, waiting for feedback
    - ifunc crash
      - Root-Analysis: https://gitee.com/aosp-riscv/working-group/issues/I5DNIJ, found rootcasue, in resloving
    - exec_argv0_null failed
      - Root-Analysis: https://gitee.com/aosp-riscv/working-group/issues/I5EGDI, resolved.
  - Articles:
    - Bazel for AOSP: https://zhuanlan.zhihu.com/p/529147848 (in chinese)
    - GNU IFUNC (RISC-V): https://zhuanlan.zhihu.com/p/532312568 (in chinese)
    - IFUNC in BIONIC: https://zhuanlan.zhihu.com/p/532885045 (in chinese)

- 2022-05-30 Status update

  - RVI upstream:
    - redo prebuilt ndk for 12: https://github.com/riscv-android-src/platform-prebuilts-ndk/pull/2
    - fixed some typo issues: https://github.com/riscv-android-src/platform-bionic/pull/20
    - fixed issue for setjmp: https://github.com/riscv-android-src/platform-bionic/pull/21
    - using 0x0001(c.addi) as padding bytes: https://github.com/riscv-android-src/platform-bionic/pull/23
    - remove TLS_SLOT_SELF for riscv: https://github.com/riscv-android-src/platform-bionic/pull/24
    - passed setjmp_DeathTest.setjmp_cookie_checksum: https://github.com/riscv-android-src/platform-bionic/pull/25
    - fixed TLS issues and pass unit tests: https://github.com/riscv-android-src/platform-bionic/pull/26
  - aosp-riscv development and bugfix:
    - PR for Disable dex2oat: https://gitee.com/aosp-riscv/platform_build/pulls/5
    - completed sync mem* from RVI upstream：https://gitee.com/aosp-riscv/platform_bionic/pulls/22
    - fix setjmp issue： https://gitee.com/aosp-riscv/platform_bionic/pulls/21
    - sync from rvi upstream：https://gitee.com/aosp-riscv/platform_bionic/pulls/19
    - added log after fixed setjmp issue：https://gitee.com/aosp-riscv/test-riscv/pulls/18
    - enable those disabled cases: https://gitee.com/aosp-riscv/test-riscv/pulls/19
    - workaround sanitize issue: https://gitee.com/aosp-riscv/platform_build_soong/pulls/7
    - sync tls from rvi: https://gitee.com/aosp-riscv/platform_bionic/pulls/24
    - fixed setjmp checksum issue: https://gitee.com/aosp-riscv/platform_bionic/pulls/23
  - Articles:
    - PR for the analysis of RenderScript in Android 12 RISCV64 porting: https://gitee.com/aosp-riscv/working-group/pulls/34
    - add article about setjmp：https://gitee.com/aosp-riscv/working-group/pulls/35

- 2022-04-28 Status update
  
  - RVI upstream:
    - [fixed issues and code cleanup for emulator](https://github.com/riscv-android-src/platform-external-qemu/pull/3)
    - [implement generate cmake-main.xxx.inc by scripts](https://github.com/riscv-android-src/platform-external-qemu/pull/4)
    - [reverted commented codes](https://github.com/riscv-android-src/platform-external-qemu/pull/7)
    - [added defconfig for ranchu](https://github.com/riscv-android-src/kernel-common/pull/1)
    - [updated with latest emulator](https://github.com/riscv-android-src/platform-prebuilts-android-emulator/pull/1)
    - [updated kernel for emulator](https://github.com/riscv-android-src/kernel-prebuilts-5.10-riscv64/pull/1)
    - [updated build guide for aosp 12](https://github.com/riscv-android-src/riscv-android/pull/2)
  - aosp-riscv development and bugfix:
    - [Updated emulator integration branch](https://gitee.com/aosp-riscv/working-group/pulls/28)
    - [Remove bcc and ld.mc in Target Base System](https://gitee.com/aosp-riscv/platform_build/pulls/4)
  - Technical articles related:
    - [added article gdb android emulator](https://gitee.com/aosp-riscv/working-group/pulls/29)
    - [added article run qemu 2.12.0](https://gitee.com/aosp-riscv/working-group/pulls/27)
    - [added article to introduce how ndk is built](https://gitee.com/aosp-riscv/working-group/pulls/26)
    - [article updated how-ndk-built](https://gitee.com/aosp-riscv/working-group/pulls/32)

- 2022-03-30 Status update 

  - Sync aosp-riscv to RVI upstream:
    - [improve vfork](https://github.com/riscv-android-src/platform-bionic/pull/19)
  - Sync aosp-riscv from RVI upstream:
    - N/A
  - aosp-riscv development and bugfix:
    - [New TARGET_PRODUCT sdk_phone_riscv64 with 64bit Support Only](https://gitee.com/aosp-riscv/platform_build/pulls/3)
    - [changed device/generic/goldfish to aosp-riscv](https://gitee.com/aosp-riscv/platform_manifest/pulls/13)
    - [pass build with m --skip-ninja for sdk_phone_riscv64-eng](https://gitee.com/aosp-riscv/device-generic-goldfish/pulls/1)
    - [sudo is not required for make_rootfs.sh](https://gitee.com/aosp-riscv/test-riscv/pulls/15)
    - [added prebuilt gdb](https://gitee.com/aosp-riscv/test-riscv/pulls/16)
    - [use gdb built for 18.04](https://gitee.com/aosp-riscv/test-riscv/pulls/17)
  - Technical articles related:
    - [added doc to introduce howto build emu](https://gitee.com/aosp-riscv/working-group/pulls/22)
    - [Add rust toolchain build instructions](https://gitee.com/aosp-riscv/working-group/pulls/23)

- 2022-03-16 Status update

  - Sync aosp-riscv to RVI upstream:
    - [squash and remove duplicated codes](https://github.com/riscv-android-src/platform-bionic/pull/17)
    - [minor bugfixes in fenv](https://github.com/riscv-android-src/platform-bionic/pull/18)
  - Sync aosp-riscv from RVI upstream:
    - [riscv64: fix fenv handling](https://github.com/aosp-riscv/platform_bionic/pull/5)
  - aosp-riscv development and bugfix:
    - [libm/riscv64: minor bugfixes in fenv](https://github.com/aosp-riscv/platform_bionic/pull/7)
    - [add argument check for fesetround()](https://gitee.com/aosp-riscv/platform_bionic/pulls/18)
    - [fixed doc link issue](https://gitee.com/aosp-riscv/test-riscv/pulls/13)
    - [fixed issue when run bionic host test](https://gitee.com/aosp-riscv/test-riscv/pulls/14)
    - [PR for Feature "setup golang develop/debug env for soong"](https://gitee.com/aosp-riscv/working-group/pulls/17)
    - [[RISCV] Pass -mno-relax to assembler when -fno-integrated-as specified](https://reviews.llvm.org/D120639)
    - [[RISCV] Generate correct ELF EFlags when .ll file has target-abi attribute](https://reviews.llvm.org/D121183)
  - Technical articles related:
    - [20220226-case-prebuilt-elf-files.md](https://github.com/aosp-riscv/working-group/pull/38)
    - [add doc to introduce how to add entry in lunch](https://gitee.com/aosp-riscv/working-group/pulls/20)
    - [add code version for doc](https://gitee.com/aosp-riscv/working-group/pulls/19)

- 2022-03-03 Status update

  - Sync aosp-riscv to RVI upstream:
    - [upgrade kernel uapi to 5.12](https://github.com/riscv-android-src/platform-bionic/pull/14)
    - [clean-up some minor faults](https://github.com/riscv-android-src/platform-bionic/pull/15)
    - [fix android unsafe frame pointer chase](https://github.com/riscv-android-src/platform-bionic/pull/16)
    - [some cleanup and restore](https://github.com/riscv-android-src/platform-build-soong/pull/3)
  - Sync aosp-riscv from RVI upstream:
    - [sync with RVI upsteam: pr#14](https://gitee.com/aosp-riscv/platform_bionic/pulls/15)
    - [sync from RVI upstream: linker_wrapper](https://gitee.com/aosp-riscv/platform_bionic/pulls/16)
    - [removed FIXME](https://gitee.com/aosp-riscv/platform_bionic/pulls/17)
    - [sync from RVI upstream, removed duplicated cflags](https://gitee.com/aosp-riscv/platform_build_soong/pulls/4)
    - [fixed format issue](https://gitee.com/aosp-riscv/platform_build_soong/pulls/5)
    - [RVI upstream sync, removed FIXME](https://gitee.com/aosp-riscv/platform_build_soong/pulls/6)
  - aosp-riscv development and bugfix:
    - [updated bionic unit test on host](https://gitee.com/aosp-riscv/test-riscv/pulls/11)
    - [optimize the test scripts](https://gitee.com/aosp-riscv/test-riscv/pulls/12)
    - [Enable create_minidebuginfo](https://github.com/aosp-riscv/platform_build_soong/pull/2)
    - [Updated dependencies needed by create_minidebuginfo (Relocated)](https://github.com/aosp-riscv/platform_art/pull/2)
    - [linux-x86/bin/create_minidebuginfo supports riscv64](https://gitee.com/aosp-riscv/platform-prebuilts-build-tools/pulls/1)
  - Technical articles related:
    - [status updated on Feb/17/2022](https://gitee.com/aosp-riscv/working-group/pulls/14)
    - [added template for articles](https://gitee.com/aosp-riscv/working-group/pulls/15)

- 2022-02-17 Status update

  - Setup bionic dynamic-link unit test and bugfix:
    - [support dynamic link tests](https://gitee.com/aosp-riscv/test-riscv/pulls/6)
    - [updated doc about test](https://gitee.com/aosp-riscv/test-riscv/pulls/7)
    - [added search path for libicu.so](https://gitee.com/aosp-riscv/test-riscv/pulls/8)
    - [added 2.log for bionic dynamic link test](https://gitee.com/aosp-riscv/test-riscv/pulls/9)
    - [added 4.log for bionic static test](https://gitee.com/aosp-riscv/test-riscv/pulls/10)
  - Sync aosp-riscv from RVI upstream:
    - [define REG_* for ucontext](https://gitee.com/aosp-riscv/platform_bionic/pulls/12)
    - [asm header files](https://gitee.com/aosp-riscv/platform_bionic/pulls/13)
    - [unify rv64 pre-processor definition](https://gitee.com/aosp-riscv/platform_bionic/pulls/14)
  - Sync aosp-riscv to RVI upstream:
    - [added stack overflow reserved bytes for rv64](https://github.com/riscv-android-src/platform-art/pull/2)
    - [removed duplicated asm riscv header file](https://github.com/riscv-android-src/platform-bionic/pull/8)
    - [unify rv64 preprocessor definition](https://github.com/riscv-android-src/platform-bionic/pull/13)
    - [upgrade kernel uapi to 5.12](https://github.com/riscv-android-src/platform-bionic/pull/14) in reviewing

- 2022-01-20 Status update
  - Bionic static unit test and bugfix:
    - [fix TLS issues](https://gitee.com/aosp-riscv/platform_bionic/pulls/5)
    - [change TLS slot organization same as that for arm](https://gitee.com/aosp-riscv/platform_bionic/pulls/6)
    - [fixed vfork](https://gitee.com/aosp-riscv/platform_bionic/pulls/7)
    - [continue vfork](https://gitee.com/aosp-riscv/platform_bionic/pulls/8)
    - [fixed memset infinite loop issue](https://gitee.com/aosp-riscv/platform_bionic/pulls/10)
    - [some changes for test env](https://gitee.com/aosp-riscv/test-riscv/pulls/3)
    - [run test with isolate mode](https://gitee.com/aosp-riscv/test-riscv/pulls/4)
    - [enabled some blocking cases](https://gitee.com/aosp-riscv/test-riscv/pulls/5)
    - [minor changes and ready for upstream](https://gitee.com/aosp-riscv/platform_bionic/pulls/9)
    - [code format improvement](https://gitee.com/aosp-riscv/platform_bionic/pulls/11)
    - [minor fix and ready for upstream](https://gitee.com/aosp-riscv/platform_build/pulls/2)
    - [minor fix and ready for upstream](https://gitee.com/aosp-riscv/platform_build_soong/pulls/3)
  - Article/Document update:
    - [continue update android kernel related knowledge](https://gitee.com/aosp-riscv/working-group/pulls/10)
    - [updated article: platform-version](https://gitee.com/aosp-riscv/working-group/pulls/11)
    - [add how to build clang for aosp](https://gitee.com/aosp-riscv/working-group/pulls/12)
  - RVI aosp 12 upstream work:
    - [RISC-V Android 12 Source code repo in github, Jan/17/2022](https://lists.riscv.org/g/sig-android/message/32),
    - [removed old seccomp txt files](https://github.com/riscv-android-src/platform-bionic/pull/3)
    - [bugfix: enable rela ifunc resolver for riscv64](https://github.com/riscv-android-src/platform-bionic/pull/4)
    - [format code style](https://github.com/riscv-android-src/platform-bionic/pull/6)

- 2021-12-23 Status update
  - Build a minimal system, run AOSP's bionic-unit-test-static on QEMU, and solve the bugs found in the test.
    - malloc.malloc_info: SIGABRT：<https://gitee.com/aosp-riscv/test-riscv/pulls/2>
    - part of stdio cases FAILED due to can not create tmpfiles: <https://gitee.com/aosp-riscv/test-riscv/pulls/2>
    - Fixed wrong mdev path issue: <https://gitee.com/aosp-riscv/test-riscv/pulls/2>
    - ifunc.*：Segmentation fault: <https://gitee.com/aosp-riscv/platform_bionic/pulls/4>; also raise PR to upstream: <https://github.com/riscv-android-src/platform-bionic/pull/1>
    - added bionic-unit-tests-static log: <https://gitee.com/aosp-riscv/test-riscv/pulls/2>
    - Added doc on how-to setup test env: <https://gitee.com/aosp-riscv/test-riscv/pulls/2>
  - Participate in OSDTConf2021 and present a report about AOSP for RISC-V community open source progress
    - report slides: <https://github.com/plctlab/PLCT-Open-Reports/blob/master/20211218-osdt2021-aosp-rv-wangchen.pdf>
    - report vedio: <https://www.bilibili.com/video/BV1Sg411w7Le>

- 2021-12-10: Bionic has been successfully compiled, and integration tests are being run using the gtest suite provided by Google.
  - Clang toolchain: added libFuzzer
    - <https://github.com/aosp-riscv/toolchain_llvm_android/pull/1>
    - <https://github.com/aosp-riscv/toolchain_llvm-project/pull/1>
    - <https://github.com/aosp-riscv/platform-prebuilts-clang-host-linux-x86/commit/5d06484b069ec0af9f0d278a3fcc2559bb6f37f4>
  - Enable soong tests during building:
    - <https://github.com/aosp-riscv/platform_build_soong/pull/1>
    - <https://github.com/aosp-riscv/platform-build-bazel/pull/1>
  - Move some patches to git:
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/10>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-runtime/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-packages-services-Car/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-packages-modules-ArtPrebuilt/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-frameworks-base/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-cts/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-clang-host-linux-x86/pulls/1>
  - Pass build with "mmm bionic":
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/7>
    - <https://gitee.com/aosp-riscv/platform-external-llvm/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_bionic/pulls/3>
    - <https://gitee.com/aosp-riscv/platform-external-scudo/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-unwinding/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-libbase/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_build_soong/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/8>
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/9>
  - Added test repo:
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/11>

- 2021-11-30 For AOSP 12 ported to RV64, the implementation of `mmm bionic/libc/ --skip-soong-tests` was successfully built, we now have libc/libm/libdl. The relevant changes have been submitted to the repositories. The details are as follows (here, some PRs are created on gitee, all commits are pushed both on github & gitee):
  - toolchain for AOSP：
    - Rust for Android:
      - compiler: add riscv64gc-linux-android target (Tier 3): <https://github.com/aosp-riscv/rust/commit/8397a76b895b586dcfc8637e98908f65df1af1f9>
      - library: add riscv64gc-linux-android support: <https://github.com/aosp-riscv/rust/commit/2eacf985415ba38018de70845c0b150885ad4d57>
      - Modified libc, added support for riscv64：<https://github.com/aosp-riscv/toolchain_rustc/pull/1>
    - Clang/llvm for Android:
      - add riscv __get_tls method: <https://github.com/aosp-riscv/toolchain_llvm-project/commit/2511f435f0c0c02f1a64d55b3b45c657dc84b050>
      - Change some repos' fetch address and revision to support riscv64: <https://github.com/aosp-riscv/platform_manifest/commit/a3446a1580947ac5d5d29e99365fdf90b0edb3ef>
  - manifest：PR: <https://gitee.com/aosp-riscv/platform_manifest/pulls/7>
  - AOSP (PRs)：
    - <https://gitee.com/aosp-riscv/platform-external-scudo/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-unwinding/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-libbase/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_build_soong/pulls/2>
    - <https://gitee.com/aosp-riscv/platform_bionic/pulls/2>
    - <https://gitee.com/aosp-riscv/platform-external-kernel-headers/pulls/1>

- 2021-11-15 For AOSP 12 ported to RV64, the implementation of `m --skip-ninja --skip-soong-tests` was successfully built. The relevant changes have been submitted to the repositories. The details are as follows (here, gitee is taken as an example, github has been mirrored):
  - manifest repository：<https://gitee.com/aosp-riscv/platform_manifest>
  - Newly added AOSP repositories:
    - art/ : <https://gitee.com/aosp-riscv/platform_art>
    - bionic/: <https://gitee.com/aosp-riscv/platform_bionic>
    - build/make/: <https://gitee.com/aosp-riscv/platform_build>
    - build/soong/: <https://gitee.com/aosp-riscv/platform_build_soong>
    - external/crosvm/: <https://gitee.com/aosp-riscv/platform_external_crosvm>
    - frameworks/av/: <https://gitee.com/aosp-riscv/platform_frameworks_av>
    - packages/modules/NeuralNetworks/: <https://gitee.com/aosp-riscv/platform-packages-modules-NeuralNetworks>
    - prebuilts/vndk/v28/: <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v28>
    - prebuilts/vndk/v29/: <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v29>
    - prebuilts/vndk/v30/: <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v30>
    - system/apex/: <https://gitee.com/aosp-riscv/platform-system-apex>
    - system/core/: <https://gitee.com/aosp-riscv/platform_system_core>
    - system/iorap/: <https://gitee.com/aosp-riscv/platform-system-iorap>
    - system/nvram/: <https://gitee.com/aosp-riscv/platform-system-nvram>
  - In addition, some repositories are currently unable to be created on gitee/github because the overall volume or some files are too large and exceed the limitaiton required by gitee/github. Therefore, we currently use patching method to save the changes, and below are these repositories involved:
    - cts/
    - frameworks/base/
    - packages/modules/ArtPrebuilt/
    - packages/services/Car/
    - prebuilts/clang/host/linux-x86/
    - prebuilts/runtime/
    The newly built patch repository is: <https://gitee.com/aosp-riscv/patches_aosp_riscv>
  - The list of PRs submitted is as follows:
    - <https://gitee.com/aosp-riscv/platform_art/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_bionic/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_build/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_build_soong/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_external_crosvm/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_frameworks_av/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-packages-modules-NeuralNetworks/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_system_core/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_system_core/pulls/2>
    - <https://gitee.com/aosp-riscv/platform-system-iorap/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-nvram/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-system-apex/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v28/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v29/pulls/1>
    - <https://gitee.com/aosp-riscv/platform-prebuilts-vndk-v30/pulls/1>
    - <https://gitee.com/aosp-riscv/patches_aosp_riscv/pulls/1>
    - <https://gitee.com/aosp-riscv/platform_manifest/pulls/6>
  - Updated tech article [“Howto build aosp-riscv” ](./articles/20211029-howto-setup-build-env.md)
  - Newed tech article ["Code analysis for soong_ui"](./articles/20211102-codeanalysis-soong_ui.md)

- 2021-10-29 Restart the AOSP migration work of PLCT lab. The new goal is to migrate AOSP 12 to RV64.
  - Setup group @ <https://github.com/aosp-riscv> (mirrored with <https://gitee.com/aosp-riscv>) with some initial repos.
  - [PR Merged] added note on howto setup build env: <https://gitee.com/aosp-riscv/working-group/pulls/1>
  - Newed tech article ["Code analysis for envsetup"](./articles/20211026-lunch.md)
  - Newed tech article ["Howto setup build environment"](./articles/20211029-howto-setup-build-env.md)

- 2021-01-xx Due to [open-source work](https://github.com/T-head-Semi/aosp-riscv) from T-Head(alibaba), we (PLCT lab) stopped the AOSP porting work in early 2021. All code repositories (except working-group) under the original <https://github.com/aosp-riscv> and <https://gitee.com/aosp-riscv> are backed up to [Gitee's aosp-riscv-bionic-porting](https://gitee.com/aosp-riscv-bionic-porting).

- 2021-01-15 Still in investigating how to add RVG64 support based on the Soong framework of AOSP. The main difficulty at present is: due to the AOSP compilation system is extremely large and complex, contains too much content required for engineering, and is intertwined and dependent on each other, so how to cleanly mask the modules that we don’t care about first and only compile those core content of the bionic library is the current focus and difficulty. There is not much clue yet, anyone is welcomed to provide help and suggestions.

- 2020-12-29 Completed the porting of dynamic linking feature for bionic, we can now support implicit dynamic link and explicit dynamic link with bionic. The make framework system is also improved. For specific changes, please refer to the `develop` branch of the bionic transplantation main repository, <https://github.com/aosp-riscv/port_bionic>. The next step is to port and modify AOSP's Soong construction system and use AOSP Soong construction system to compile the bionic library and related applications for RISC-V.

- 2020-12-15
    - Started to port the dynamic link functionality for bionic. Currently, the compilation and linking of `libc.so` and `linker` have been completed, but still can not work well. We are still under debugging. Anybody familiar with the dynamic linker are welcomed to provide your suggestions. For specific changes, please refer to the `develop` branch of the bionic transplantation main repository, <https://github.com/aosp-riscv/port_bionic>, and its corresponding submodule repository.
    - The focus of further work is still to achieve dynamic link support, and the current implementation flaws are felt to be larger than static links. In addition, we will continue to improve the bionic function and try to transplant the Soong construction system of AOSP, and support the use of AOSP Soong construction system to compile the bionic library and related applications for RISC-V.

- 2020-11-30 Completed the porting of shell and init. Currently, the bionic libc.a library can work, and a minimal Android root file system can be generated by statically linking against it. The rootfs can be launched and run on qemu-system-riscv64 and supports running some basic operation commands. In addition, the original make framework system has been improved to prepare for the next stage to further support dynamic linking. For specific changes, please refer to the `develop` branch of the bionic transplantation main repository, <https://github.com/aosp-riscv/port_bionic>, and its corresponding submodule repository. For more detailed introduction, please refer to another introduction article: ["Create a minimal Android system for RISC-V"] (https://plctlab.github.io/aosp/create-a-minimal-android-system-for-riscv.html). The next step is to improve the dynamic linking support, continue to improve the bionic function and try to transplant the Soong construction system of AOSP, and support the use of the Soong construction system of AOSP to compile the bionic library and related applications for RISC-V.

- 2020-11-13 Further improve the libc static library of bionic. Currently using v10.0.1 llvm/clang to compile, and using the GNU RISC-V version of ld based on bionic's libc_static, crtbegin_static and crtend.o to achieve static linking and executable program generation. On this basis, we transplanted and compiled the toybox that comes with AOSP and made a minimal rootfs (shell and init still use busybox compiled for glibc). At present, the rootfs can be started and run on qemu-system-riscv64. You can run some commands, but some commands will report errors when executed. For specific changes, please refer to the commit comment of the `develop` branch of the bionic porting main repository, <https://github.com/aosp-riscv/port_bionic>, and the commit comments of the `develop` branch of its submodule repositories. The next step will be to continue to improve the libc of bionic and the self-made minimal root file system.

- 2020-10-30 The make framework has been improved to support the compilation and generation of libc static libraries, as well as `crtbegin.o` and `crtend.o`. Currently, the statically compiled executable program can jump from the kernel to the main function and ensure that `argv` and `envp` are read correctly. The porting of other TLS and syscall parts is still ongoing. For specific changes, please refer to the `develop` branch of <https://github.com/aosp-riscv/port_bionic>.

- 2020-10-16 Simplify the build framework of the AOSP bionic library to make, and submit the preliminary porting work to the develop branch of <https://github.com/aosp-riscv/port_bionic>. Currently, the project repository depends on two sub-repositories: <https://github.com/aosp-riscv/platform_bionic> and <https://github.com/aosp-riscv/external_jemalloc_new> (Because the changes are still drafts, the current changes to AOSP are all placed on their respective `develop` branches. Another note: the current porting work is based off `android-10.0.0_r39` tag version based on AOSP). The next step is to continue to improve the make project and make changes to the bionic code. The first goal is to implement `libc.a`.

- 2020-10-01 Using the latest AOSP kernel and configuration, with GNU and LLVM/Clang tool-chain for RISC-V, we successfully compiled and booted the kernel on QEMU. The only problem is: after mounting the minimum rootfs, we encounter a "Segmentation Fault" error. Still debugging this issue. In addition, we began to study how to port the bionic library of AOSP to RISC-V. The current work is to port the AOSP building framework for bionic.

- 2020-09-15 We launched the task to port the AOSP kernel to RISC-V. After some research, we decided to base it off the branch `android-5.4-stable` of AOSP common kernels and we are working on it now.

- 2020-09-01 We fixed the errors in build dir and prebuilts dir when using the 'mmm art/compiler' to generate the ninja files for the compiler module. We are debugging the errors in the system dir and cts dir.

- 2020-08-15 We have added the lunch menu for RISC-V, and successfully loaded the menu. After that, we added the RISC-V config files for build/make and build/soong, We are debugging the RISC-V config files in build/soong.

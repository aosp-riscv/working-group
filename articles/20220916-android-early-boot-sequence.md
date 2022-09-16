![](./diagrams/android.png)

文章标题：**学习笔记: Android Early Init Boot Sequence**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>


本文总结了 Andorid 系统所谓 Early Init Boot 的 启动顺序。

基于 AOSP 12 for riscv ( <https://github.com/riscv-android-src>, branch: `riscv64-android-12.0.0_dev`)

文章大纲

<!-- TOC -->

- [1. 参考：](#1-参考)
- [2. Android boot 的内核部分](#2-android-boot-的内核部分)
- [3. Android boot 的用户态部分](#3-android-boot-的用户态部分)
	- [3.1. first stage init](#31-first-stage-init)
	- [3.2. SELinux setup](#32-selinux-setup)
	- [3.3. second stage init](#33-second-stage-init)

<!-- /TOC -->

# 1. 参考：


# 2. Android boot 的内核部分

在内核引导过程中，创建根文件系统的过程中经常会需要运行一些用户态程序。这些用户态程序一般会存放在外设的文件系统上，所以为了挂载和访问这些外设上的文件系统，内核必须满足某些条件。首先内核需要相应的设备驱动程序来访问根文件系统所在的外设（譬如通过 SCSI 驱动程序访问硬盘、软驱或者光驱等），考虑到外设类型可能并不固定以及设备要支持热插拔，内核是否要集成更多的外设驱动变得很棘手；此外内核还必须包含读取各种文件系统所需的代码（ext2、reiserfs、romfs 等），而越来越多的文件系统也对内核提出挑战；更有甚者，外设上的文件系统可能会被压缩乃至加密，为此直接挂载和访问这些外设上的文件系统内核还需要支持更多的功能。所有的这些问题都会直接导致一个内核为了变得更通用，需要变得更强大，同时内核的体积也会变得非常大。

Linux 的解决方法是在挂载真正的根文件系统之前先挂载一个临时的在内存里实现的小型根文件系统，也就是整个挂载根文件系统的过程分为两个阶段，即第一阶段先挂载一个基于内存的小型的根文件系统后执行一些用户态程序（譬如创建文件目录，加载一些驱动模块），在逐渐扩展根文件系统到一定程度后再执行第二阶段的挂载真实的外设根文件系统。

这个内存中的小型根文件系统中的内容（譬如应用程序或者目录结构）也存放在存储内核的外设中（可以打包在内核一起也可以和内核分开存放）。这个外设由 Bootloader 负责从引导介质上访问，对内核没有特别的驱动要求。Bootloader 会在加载内核的同时，将该小型根文件系统读入内存并将其交给内核。内核实现这个临时内存文件系统的方式经历了从 initrd 到 目前的 initramfs 的转变。我们现在暂只讨论 initramfs 的情况。

在 initramfs 的情况下，外设上的小型根文件系统实现为一个压缩形式的 cpio 格式的文件，内核在引导阶段会先创建虚拟根文件系统 VFS，同时创建了 rootfs 的根目录 `/`，并为 `/` 先基于 tmpfs 挂载一个内存中的文件系统， 然后将基于 cpio 格式的小型根文件系统（来自 Bootloader 的传递）中的内容解压后在 tmpfs 上进行重建，从而创建一个根文件系统。

根据内核 initramfs 的设计，内存根文件系统创建成功后，默认将切换处理器到用户态去执行 `/init` 这个文件。此时基本上就完成了系统引导过程中内核态部分的工作，剩下的工作继续交由用户态的部分来完成。

基于以上的理论背景，Andriod 作为一个基于 Linux 开发的系统，概莫能外也遵循这个流程，所以为支持 Linux 的 initramfs 机制，AOSP 也需要提供一个小型的根文件系统并提供给 Bootloader。本文只讨论和内核分开处理的方式，所以 AOSP
需要为 Bootloader 除了提供一个 kernel 的 image 外还需要提供另外一个 image，包含了将在第一阶段加载到内存中的小型根文件系统，在习惯上我们会把这个内存中的小型文件系统叫做 ramdisk，这是沿袭了早期 initrd 的说法 （initrd 就是 initial ram disk 的缩写）。我们看到在 AOSP 的 out 目录下会生成一个 `ramdisk.img`，就是这个东西。看一下和 `ramdisk.img` 对应的打包目录，如下：

```bash
$ ls out/target/product/emulator_riscv64/ramdisk -l
total 2788
drwxrwxr-x 2 wangchen wangchen    4096 Sep 12 22:41 debug_ramdisk
drwxrwxr-x 2 wangchen wangchen    4096 Sep 12 22:41 dev
-rwxrwxr-x 1 wangchen wangchen 2819360 Sep 12 22:41 init
drwxrwxr-x 2 wangchen wangchen    4096 Sep 12 22:41 metadata
drwxrwxr-x 2 wangchen wangchen    4096 Sep 12 22:41 mnt
drwxrwxr-x 2 wangchen wangchen    4096 Sep 12 22:41 proc
drwxrwxr-x 2 wangchen wangchen    4096 Sep 12 22:41 second_stage_resources
drwxrwxr-x 2 wangchen wangchen    4096 Sep 12 22:41 sys
drwxrwxr-x 3 wangchen wangchen    4096 Sep 12 22:41 system
```

我们会发现和我们平时使用 Linux 时见到的 `/` 文件系统下的东西很像，但是目录少了很多，而且大部分都是空的，所以这个 ramdisk 很小，ramdisk.img 就 1M 多一点。虽然内容不多，但是这里面一定有一个 `init` 程序，这也是内核在用户态执行的第一个程序。kernel 在 boot 阶段的 console log 和本节描述有关的打印如下：

```bash
[    0.402654] Unpacking initramfs...
[    0.473614] Freeing initrd memory: 1544K
......
[    3.106191] Run /init as init process
```
下面我们就来看看 AOSP 下用户态的引导过程是如何进行的。其实下面的内容才是本文标题所关心的核心，但是往前推一下了解一下内核的工作对我们理解用户态的工作也有好处。

备注，采用 emulator 启动 android 时，使用的 ramdisk 镜像文件是 `ramdisk-qemu.img`。

# 3. Android boot 的用户态部分

这部分的总体描述可以参考 AOSP 源码下 `system/core/init/README.md` 的最后一节 "Early Init Boot Sequence"。所以我们也把这个过程称之为 "Early Init Boot Sequence"，就是这个道理。

我们这里对照该段文字、再加上代码和 console 打印一起理解一下（注，console log 是运行 emulator 的结果）。

> The early init boot sequence is broken up into three stages: first stage init, SELinux setup, and
> second stage init.

官方说法，early init boot 分为三个阶段：

- first stage init
- SELinux setup
- second stage init

## 3.1. first stage init

引用 `system/core/init/README.md` 的最后一节 "Early Init Boot Sequence" 的文字：

> First stage init is responsible for setting up the bare minimum requirements to load the rest of the
> system. Specifically this includes mounting /dev, /proc, mounting 'early mount' partitions (which
> needs to include all partitions that contain system code, for example system and vendor), and moving
> the system.img mount to / for devices with a ramdisk.

> Note that in Android Q, system.img always contains TARGET_ROOT_OUT and always is mounted at / by the
> time first stage init finishes. Android Q will also require dynamic partitions and therefore will
> require using a ramdisk to boot Android. The recovery ramdisk can be used to boot to Android instead
> of a dedicated ramdisk as well. 

> First stage init has three variations depending on the device configuration:
> 1) For system-as-root devices, first stage init is part of /system/bin/init and a symlink at /init
> points to /system/bin/init for backwards compatibility. These devices do not need to do anything to
> mount system.img, since it is by definition already mounted as the rootfs by the kernel. 

> 2) For devices with a ramdisk, first stage init is a static executable located at /init. These
> devices mount system.img as /system then perform a switch root operation to move the mount at
> /system to /. The contents of the ramdisk are freed after mounting has completed.

> 3) For devices that use recovery as a ramdisk, first stage init it contained within the shared init
> located at /init within the recovery ramdisk. These devices first switch root to
> /first_stage_ramdisk to remove the recovery components from the environment, then proceed the same
> as 2). Note that the decision to boot normally into Android instead of booting
> into recovery mode is made if androidboot.force_normal_boot=1 is present in the
> kernel commandline.

对于我使用的 AOSP 12 的 `sdk_phone64_riscv64` + emulator 启动方式，我理解是走的 variation 2。因为 enable 了动态分区机制（`PRODUCT_USE_DYNAMIC_PARTITIONS` 为 true），所以肯定不支持 `system-as-root`，即 variation 1。emulator 启动时的 ramdisk 使用的是 `ramdisk-qemu.img` 而不是 `boot.img` 所以暂不考虑 variation 3. 

first stage init 使用的 init 程序源码构建参考 `system/core/init/Android.mk`

```makefile
LOCAL_MODULE := init_first_stage
```

入口 `main()` 函数定义在 `system/core/init/first_stage_main.cpp`

```cpp
int main(int argc, char** argv) {
    return android::init::FirstStageMain(argc, argv);
}
```

`FirstStageMain()` 函数定义在 `system/core/init/first_stage_init.cpp`

`FirstStageMain()` 的执行大致流程对应 console log 如下

```
[    3.167125] init: init first stage started!
[    3.169888] init: Unable to open /lib/modules, skipping module loading.
[    3.175761] init: Copied ramdisk prop to /second_stage_resources/system/etc/ramdisk/build.prop
[    3.183346] init: [libfs_mgr]ReadFstabFromDt(): failed to read fstab from dt
[    3.199531] init: Using Android DT directory /proc/device-tree/firmware/android/
[    3.497670] init: [libfs_mgr]superblock s_max_mnt_count:65535,/dev/block/platform/20000c00.virtio_mmio/by-name/metadata
[    3.498825] init: [libfs_mgr]Filesystem on /dev/block/platform/20000c00.virtio_mmio/by-name/metadata was not cleanly shutdown; state flags: 0x1, incompat feature flags: 0x46
[    3.503212] EXT4-fs (vdb1): Ignoring removed nomblk_io_submit option
[    3.535247] EXT4-fs (vdb1): recovery complete
[    3.537300] EXT4-fs (vdb1): mounted filesystem with ordered data mode. Opts: errors=remount-ro,nomblk_io_submit
[    3.538334] init: [libfs_mgr]check_fs(): mount(/dev/block/platform/20000c00.virtio_mmio/by-name/metadata,/metadata,ext4)=0: Success
[    3.562801] init: [libfs_mgr]check_fs(): unmount(/metadata) succeeded
[    3.563977] init: [libfs_mgr]Not running /system/bin/e2fsck on /dev/block/vdb1 (executable not in system image)
[    3.587200] EXT4-fs (vdb1): mounted filesystem with ordered data mode. Opts: 
[    3.587907] init: [libfs_mgr]__mount(source=/dev/block/platform/20000c00.virtio_mmio/by-name/metadata,target=/metadata,type=ext4)=0: Success
[    3.593888] init: Failed to copy /avb into /metadata/gsi/dsu/avb/: No such file or directory
[    3.617583] init: [libfs_mgr]Created logical partition system on device /dev/block/dm-0
[    3.623456] init: [libfs_mgr]Created logical partition system_ext on device /dev/block/dm-1
[    3.629855] init: [libfs_mgr]Created logical partition product on device /dev/block/dm-2
[    3.635669] init: [libfs_mgr]Created logical partition vendor on device /dev/block/dm-3
[    3.636768] init: DSU not detected, proceeding with normal boot
[    3.645721] init: [libfs_mgr]superblock s_max_mnt_count:65535,/dev/block/dm-0
[    3.671613] EXT4-fs (dm-0): mounted filesystem without journal. Opts: barrier=1
[    3.672255] init: [libfs_mgr]__mount(source=/dev/block/dm-0,target=/system,type=ext4)=0: Success
[    3.680867] init: Switching root to '/system'
[    3.696196] init: [libfs_mgr]superblock s_max_mnt_count:65535,/dev/block/dm-3
[    3.717937] EXT4-fs (dm-3): mounted filesystem without journal. Opts: barrier=1
[    3.718569] init: [libfs_mgr]__mount(source=/dev/block/dm-3,target=/vendor,type=ext4)=0: Success
[    3.726724] init: [libfs_mgr]superblock s_max_mnt_count:65535,/dev/block/dm-2
[    3.748278] EXT4-fs (dm-2): mounted filesystem without journal. Opts: barrier=1
[    3.748869] init: [libfs_mgr]__mount(source=/dev/block/dm-2,target=/product,type=ext4)=0: Success
[    3.756118] init: [libfs_mgr]superblock s_max_mnt_count:65535,/dev/block/dm-1
[    3.776646] EXT4-fs (dm-1): mounted filesystem without journal. Opts: barrier=1
[    3.777748] init: [libfs_mgr]__mount(source=/dev/block/dm-1,target=/system_ext,type=ext4)=0: Success
[    3.792771] init: Skipped setting INIT_AVB_VERSION (not in recovery mode)
```

我们结合 `FirstStageMain()` 函数的源码看一下 first stage init 所做的主要工作，方便起见，说明直接内嵌注释了。


```cpp
int FirstStageMain(int argc, char** argv) {
    // 一堆 CHECKCALL 操作，主要是在 initramfs 中创建文件和目录
    // 特别是 /dev 下的操作为后面继续挂载第二阶段的根文件系统做准备。
    // 此处省略代码 ......

// [    3.167125] init: init first stage started!
    LOG(INFO) << "init first stage started!";

    // 此处省略代码 ......

    if (access(kBootImageRamdiskProp, F_OK) == 0) {
        std::string dest = GetRamdiskPropForSecondStage();
        std::string dir = android::base::Dirname(dest);
        std::error_code ec;
        if (!fs::create_directories(dir, ec) && !!ec) {
            LOG(FATAL) << "Can't mkdir " << dir << ": " << ec.message();
        }
        if (!fs::copy_file(kBootImageRamdiskProp, dest, ec)) {
            LOG(FATAL) << "Can't copy " << kBootImageRamdiskProp << " to " << dest << ": "
                       << ec.message();
        }
// [    3.175761] init: Copied ramdisk prop to /second_stage_resources/system/etc/ramdisk/build.prop
        LOG(INFO) << "Copied ramdisk prop to " << dest;
    }

    // 此处省略代码 ......

    if (ForceNormalBoot(cmdline, bootconfig)) {
        mkdir("/first_stage_ramdisk", 0755);
        // SwitchRoot() must be called with a mount point as the target, so we bind mount the
        // target directory to itself here.
        if (mount("/first_stage_ramdisk", "/first_stage_ramdisk", nullptr, MS_BIND, nullptr) != 0) {
            LOG(FATAL) << "Could not bind mount /first_stage_ramdisk to itself";
        }
// [    3.680867] init: Switching root to '/system'
	// 可以确定的是在 SwitchRoot 之前系统已经完成了第二阶段的根文件系统的挂载，这个可以从 console log 的
	// [    3.183346] ~ [    3.672255] 看出来。
	// emulator 加载的是 system-qemu.img，这个 image 由 vbmeta.img 和 super.img 组成
	// 而 super.img 源自 system.img + system_ext.img + product.img + vendor.img, 所以我们看到 console log 中
// [    3.617583] init: [libfs_mgr]Created logical partition system on device /dev/block/dm-0
// [    3.623456] init: [libfs_mgr]Created logical partition system_ext on device /dev/block/dm-1
// [    3.629855] init: [libfs_mgr]Created logical partition product on device /dev/block/dm-2
// [    3.635669] init: [libfs_mgr]Created logical partition vendor on device /dev/block/dm-3
        // 结合最终系统的 df 显示:
        // /dev/block/dm-0      727236 661676     65560  91% /	
	// /dev/block/dm-3       87036  86764       272 100% /vendor
        // /dev/block/dm-2      102912 102596       316 100% /product
        // /dev/block/dm-1      127624 127220       404 100% /system_ext
	// 这体现了 dynamic partition 的运作机制
	// 所以此后 exec 执行 /system/bin/init 是执行的第二阶段的 init
        SwitchRoot("/first_stage_ramdisk");
    }

    if (!DoFirstStageMount(!created_devices)) {
        LOG(FATAL) << "Failed to mount required partitions early ...";
    }

    struct stat new_root_info;
    if (stat("/", &new_root_info) != 0) {
        PLOG(ERROR) << "Could not stat(\"/\"), not freeing ramdisk";
        old_root_dir.reset();
    }

    if (old_root_dir && old_root_info.st_dev != new_root_info.st_dev) {
        FreeRamdisk(old_root_dir.get(), old_root_info.st_dev);
    }

// [    3.792771] init: Skipped setting INIT_AVB_VERSION (not in recovery mode)
    SetInitAvbVersionInRecovery();

    setenv(kEnvFirstStageStartedAt, std::to_string(start_time.time_since_epoch().count()).c_str(),
           1);

    const char* path = "/system/bin/init";
    const char* args[] = {path, "selinux_setup", nullptr};
    auto fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
    // 第二阶段的根文件系统挂载完毕，所以可以执行 /system/bin/init
    // 这个 init 是第二阶段的 init，带入的参数是 "selinux_setup"
    // 此时 first stage init 阶段结束, 进入下一个阶段 "SELinux setup"
    // 注意是 execv，所以进程号不变，PID 还是 1。
    execv(path, const_cast<char**>(args));

    // execv() only returns if an error happened, in which case we
    // panic and never fall through this conditional.
    PLOG(FATAL) << "execv(\"" << path << "\") failed";

    return 1;
}
```

## 3.2. SELinux setup

引用 `system/core/init/README.md` 的最后一节 "Early Init Boot Sequence" 的文字：

> Once first stage init finishes it execs /system/bin/init with the "selinux_setup" argument. This
> phase is where SELinux is optionally compiled and loaded onto the system. selinux.cpp contains more
> information on the specifics of this process.

严格讲 "SELinux setup" 阶段也是实现在 `/system/bin/init`。但这个 init 和 "first stage init" 执行的 `/init` 并不是同一个程序，这里的程序的构建由 `system/core/init/Android.bp` 定义：

```json
cc_binary {
    name: "init_second_stage",
    recovery_available: true,
    stem: "init",
    defaults: ["init_defaults"],
    static_libs: ["libinit"],
    required: [
        "e2fsdroid",
        "init.rc",
        "mke2fs",
        "sload_f2fs",
        "make_f2fs",
        "ueventd.rc",
    ],
    srcs: ["main.cpp"],
    symlinks: ["ueventd"],
    target: {
        recovery: {
            cflags: ["-DRECOVERY"],
            exclude_static_libs: [
                "libxml2",
            ],
            exclude_shared_libs: [
                "libbinder",
                "libutils",
            ],
        },
    },
    visibility: ["//packages/modules/Virtualization/microdroid"],
}
```

所以这里实现 init 的对应源文件是 `system/core/init/main.cpp`，构建的输出目录在 `out/target/product/emulator_riscv64/system/bin/init`

"SELinux setup" 阶段执行的输出 console log 如下：

```
[    4.096565] init: Opening SELinux policy
[    4.099845] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #10!!!
[    4.138316] init: Loading SELinux policy
[    4.212139] SELinux:  Permission nlmsg_getneigh in class netlink_route_socket not defined in policy.
[    4.212767] SELinux:  Permission bpf in class capability2 not defined in policy.
[    4.213079] SELinux:  Permission checkpoint_restore in class capability2 not defined in policy.
[    4.213470] SELinux:  Permission bpf in class cap2_userns not defined in policy.
[    4.213806] SELinux:  Permission checkpoint_restore in class cap2_userns not defined in policy.
[    4.214492] SELinux: the above unknown classes and permissions will be denied
[    4.236933] SELinux:  policy capability network_peer_controls=1
[    4.237228] SELinux:  policy capability open_perms=1
[    4.237454] SELinux:  policy capability extended_socket_class=1
[    4.237725] SELinux:  policy capability always_check_network=0
[    4.237975] SELinux:  policy capability cgroup_seclabel=0
[    4.238216] SELinux:  policy capability nnp_nosuid_transition=1
[    4.238467] SELinux:  policy capability genfs_seclabel_symlinks=0
[    4.238745] SELinux:  policy capability ioctl_skip_cloexec=0
[    4.423986] audit: type=1403 audit(4.408:2): auid=4294967295 ses=4294967295 lsm=selinux res=1
[    4.451320] selinux: SELinux: Loaded file_contexts
[    4.452087] selinux: 
```

我们看一下源码：

```cpp
int main(int argc, char** argv) {
#if __has_feature(address_sanitizer)
    __asan_set_error_report_callback(AsanReportCallback);
#endif
    // Boost prio which will be restored later
    setpriority(PRIO_PROCESS, 0, -20);
    if (!strcmp(basename(argv[0]), "ueventd")) {
        return ueventd_main(argc, argv);
    }

    if (argc > 1) {
        if (!strcmp(argv[1], "subcontext")) {
            android::base::InitLogging(argv, &android::base::KernelLogger);
            const BuiltinFunctionMap& function_map = GetBuiltinFunctionMap();

            return SubcontextMain(argc, argv, &function_map);
        }

        if (!strcmp(argv[1], "selinux_setup")) {
            return SetupSelinux(argv);
        }

        if (!strcmp(argv[1], "second_stage")) {
            return SecondStageMain(argc, argv);
        }
    }

    return FirstStageMain(argc, argv);
}
```

根据 `FirstStageMain()` 函数最后调用 `/system/bin/init` 带的参数 `"selinux_setup"`，我们知道这里会走调用 `SetupSelinux()` 函数。`SetupSelinux()` 函数定义在 `system/core/init/selinux.cpp`


```cpp
int SetupSelinux(char** argv) {
    SetStdioToDevNull(argv);
    InitKernelLogging(argv);

    if (REBOOT_BOOTLOADER_ON_PANIC) {
        InstallRebootSignalHandlers();
    }

    boot_clock::time_point start_time = boot_clock::now();

    MountMissingSystemPartitions();

    SelinuxSetupKernelLogging();

    LOG(INFO) << "Opening SELinux policy";

    // Read the policy before potentially killing snapuserd.
    std::string policy;
    ReadPolicy(&policy);

    auto snapuserd_helper = SnapuserdSelinuxHelper::CreateIfNeeded();
    if (snapuserd_helper) {
        // Kill the old snapused to avoid audit messages. After this we cannot
        // read from /system (or other dynamic partitions) until we call
        // FinishTransition().
        snapuserd_helper->StartTransition();
    }

    LoadSelinuxPolicy(policy);

    if (snapuserd_helper) {
        // Before enforcing, finish the pending snapuserd transition.
        snapuserd_helper->FinishTransition();
        snapuserd_helper = nullptr;
    }

    SelinuxSetEnforcement();

    // We're in the kernel domain and want to transition to the init domain.  File systems that
    // store SELabels in their xattrs, such as ext4 do not need an explicit restorecon here,
    // but other file systems do.  In particular, this is needed for ramdisks such as the
    // recovery image for A/B devices.
    if (selinux_android_restorecon("/system/bin/init", 0) == -1) {
        PLOG(FATAL) << "restorecon failed of /system/bin/init failed";
    }

    setenv(kEnvSelinuxStartedAt, std::to_string(start_time.time_since_epoch().count()).c_str(), 1);

    const char* path = "/system/bin/init";
    const char* args[] = {path, "second_stage", nullptr};
    execv(path, const_cast<char**>(args));

    // execv() only returns if an error happened, in which case we
    // panic and never return from this function.
    PLOG(FATAL) << "execv(\"" << path << "\") failed";

    return 1;
}
```

具体过程我这里暂时不深入分析。我们只关注这个函数的最后一段，看上去再次执行 `/system/bin/init` ，只不过这次带入的参数是 `"second_stage"`，标志 "SELinux setup" 阶段结束，继续进入 "second stage init" 阶段。注意是 execv，所以进程号不变，PID 还是 1。

## 3.3. second stage init

引用 `system/core/init/README.md` 的最后一节 "Early Init Boot Sequence" 的文字：

> Lastly once that phase finishes, it execs /system/bin/init again with the "second_stage"
> argument. At this point the main phase of init runs and continues the boot process via the init.rc
> scripts.

当执行 `/system/bin/init`，并且带入的参数是 `"second_stage"` 时，调用 `SecondStageMain()` 函数。

console log 上从 `init: init second stage started!` 开始往下都是这个阶段的输出，我就不列了，会很长，主要结合代码总结一下：


```cpp
int SecondStageMain(int argc, char** argv) {
    // 省略 ......

    LOG(INFO) << "init second stage started!";

    // 省略 ......

    // 初始化 Andorid 的 property 设置
    // log 中 诸如如下打印
    // Overriding previous property .....
    // Setting product property ......
    PropertyInit();

    // Umount second stage resources after property service has read the .prop files.
    UmountSecondStageRes();

    // Umount the debug ramdisk after property service has read the .prop files when it means to.
    if (load_debug_prop) {
        UmountDebugRamdisk();
    }

    // Mount extra filesystems required during second stage init
    MountExtraFilesystems();

    // Now set up SELinux for second stage.
    SelinuxSetupKernelLogging();
    SelabelInitialize();
    SelinuxRestoreContext();

    Epoll epoll;
    if (auto result = epoll.Open(); !result.ok()) {
        PLOG(FATAL) << result.error();
    }

    InstallSignalFdHandler(&epoll);
    InstallInitNotifier(&epoll);
    // 开启属性服务
    StartPropertyService(&property_fd);

    // Make the time that init stages started available for bootstat to log.
    RecordStageBoottimes(start_time);

    // Set libavb version for Framework-only OTA match in Treble build.
    if (const char* avb_version = getenv("INIT_AVB_VERSION"); avb_version != nullptr) {
        SetProperty("ro.boot.avb_version", avb_version);
    }
    unsetenv("INIT_AVB_VERSION");

    fs_mgr_vendor_overlay_mount_all();
    export_oem_lock_status();
    MountHandler mount_handler(&epoll);
    SetUsbController();

    const BuiltinFunctionMap& function_map = GetBuiltinFunctionMap();
    Action::set_function_map(&function_map);

    // 对应 log：[    5.630413] init: SetupMountNamespaces done
    if (!SetupMountNamespaces()) {
        PLOG(FATAL) << "SetupMountNamespaces failed";
    }

    InitializeSubcontext();

    ActionManager& am = ActionManager::GetInstance();
    ServiceList& sm = ServiceList::GetInstance();

    // 开始解析 init rc 文件
    // 参考另一篇笔记 《Android Init Language 学习笔记》: 
    // https://gitee.com/aosp-riscv/working-group/blob/master/articles/20220915-andorid-init-language.md
    // 首先搜索 /system/etc/init/hw/init.rc 这个文件
    // 然后依次搜索这些目录下的 rc 文件
    // - /system/etc/init
    // - /system_ext/etc/init
    // - /vendor/etc/init
    // - /odm/etc/init
    // - /product/etc/init
    // log 中会打印
    // init: Parsing directory
    // init: Parsing file
    // init: Added - 对应 import
    // 都是和这个有关
    // 所有解析后的数据在内存中缓存，但此时还未触发任何的 action
    LoadBootScripts(am, sm);

    // Turning this on and letting the INFO logging be discarded adds 0.2s to
    // Nexus 9 boot time, so it's disabled by default.
    if (false) DumpState();

    // Make the GSI status available before scripts start running.
    auto is_running = android::gsi::IsGsiRunning() ? "1" : "0";
    SetProperty(gsi::kGsiBootedProp, is_running);
    auto is_installed = android::gsi::IsGsiInstalled() ? "1" : "0";
    SetProperty(gsi::kGsiInstalledProp, is_installed);

    // QueueBuiltinAction 函数会在 init 处理 queue 中添加 action，类似于我们在 rc 文件中手写 action
    // 但这种调用 QueueBuiltinAction 添加的 action 不会等 trigger，而是在调用 QueueBuiltinAction 的
    // 同时就会被触发执行，可以参考 log 中的
    // [    5.890178] init: processing action (SetupCgroups) from (<Builtin Action>:0)
    // ......
    // [    5.958101] init: processing action (SetKptrRestrict) from (<Builtin Action>:0)
    // [    5.964838] init: processing action (TestPerfEventSelinux) from (<Builtin Action>:0)
    // ......
    am.QueueBuiltinAction(SetupCgroupsAction, "SetupCgroups");
    am.QueueBuiltinAction(SetKptrRestrictAction, "SetKptrRestrict");
    am.QueueBuiltinAction(TestPerfEventSelinuxAction, "TestPerfEventSelinux");
    // QueueEventTrigger 会触发 rc 文件中定义的 early-init 被执行
    // 可以认为从现在开始作为一个入口，前面在 LoadBootScripts 阶段解析保存的 actions 会
    // 被渐次 trigger 执行
    am.QueueEventTrigger("early-init");

    // Queue an action that waits for coldboot done so we know ueventd has set up all of /dev...
    am.QueueBuiltinAction(wait_for_coldboot_done_action, "wait_for_coldboot_done");
    // ... so that we can start queuing up actions that require stuff from /dev.
    am.QueueBuiltinAction(SetMmapRndBitsAction, "SetMmapRndBits");
    Keychords keychords;
    am.QueueBuiltinAction(
            [&epoll, &keychords](const BuiltinArguments& args) -> Result<void> {
                for (const auto& svc : ServiceList::GetInstance()) {
                    keychords.Register(svc->keycodes());
                }
                keychords.Start(&epoll, HandleKeychord);
                return {};
            },
            "KeychordInit");

    // 触发 rc 文件中定义的 init 被执行
    // Trigger all the boot actions to get us started.
    am.QueueEventTrigger("init");

    // 正常 boot 过程中只要没有进入充电模式，都是直接触发 rc 文件中定义的 late-init 被执行
    // Don't mount filesystems or start core system services in charger mode.
    std::string bootmode = GetProperty("ro.bootmode", "");
    if (bootmode == "charger") {
        am.QueueEventTrigger("charger");
    } else {
        am.QueueEventTrigger("late-init");
    }

    // Run all property triggers based on current state of the properties.
    am.QueueBuiltinAction(queue_property_triggers_action, "queue_property_triggers");

    // 至此差不多 init 阶段基于 rc 文件的初始化过程基本结束
    // 可以看出来一个正常的 boot 过程经历了 "early-init" -> "init" -> "late-init"

    // init 进程在完成初始化后并不会退出，而是进入一个 while 死循环, 继续负责如下功能
    // - 系统 shutdown 的收尾工作
    // - 回收 init 的子进程
    // - 待补充 ......
    // Restore prio before main loop
    setpriority(PRIO_PROCESS, 0, 0);
    while (true) {
        // By default, sleep until something happens.
        auto epoll_timeout = std::optional<std::chrono::milliseconds>{};

        auto shutdown_command = shutdown_state.CheckShutdown();
        if (shutdown_command) {
            LOG(INFO) << "Got shutdown_command '" << *shutdown_command
                      << "' Calling HandlePowerctlMessage()";
            HandlePowerctlMessage(*shutdown_command);
            shutdown_state.set_do_shutdown(false);
        }

        if (!(prop_waiter_state.MightBeWaiting() || Service::is_exec_service_running())) {
            am.ExecuteOneCommand();
        }
        if (!IsShuttingDown()) {
            auto next_process_action_time = HandleProcessActions();

            // If there's a process that needs restarting, wake up in time for that.
            if (next_process_action_time) {
                epoll_timeout = std::chrono::ceil<std::chrono::milliseconds>(
                        *next_process_action_time - boot_clock::now());
                if (*epoll_timeout < 0ms) epoll_timeout = 0ms;
            }
        }

        if (!(prop_waiter_state.MightBeWaiting() || Service::is_exec_service_running())) {
            // If there's more work to do, wake up again immediately.
            if (am.HasMoreCommands()) epoll_timeout = 0ms;
        }

        auto pending_functions = epoll.Wait(epoll_timeout);
        if (!pending_functions.ok()) {
            LOG(ERROR) << pending_functions.error();
        } else if (!pending_functions->empty()) {
            // We always reap children before responding to the other pending functions. This is to
            // prevent a race where other daemons see that a service has exited and ask init to
            // start it again via ctl.start before init has reaped it.
            ReapAnyOutstandingChildren();
            for (const auto& function : *pending_functions) {
                (*function)();
            }
        }
        if (!IsShuttingDown()) {
            HandleControlMessages();
            SetUsbController();
        }
    }

    return 0;
}
```

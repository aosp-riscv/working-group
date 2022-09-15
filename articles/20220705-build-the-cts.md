![](./diagrams/android-riscv.png)

文章标题：**搭建 CTS 环境**

- 作者：吴德华
- 联系方式：<wudehua2013@163.com> / <616653241@qq.com>

文章大纲
<!-- TOC -->

- [1. 搭建 Android 环境](#1-搭建-android-环境)
- [2. 运行 android 模拟器](#2-运行-android-模拟器)
- [3. 编译 cts](#3-编译-cts)
- [4. 跑 cts](#4-跑-cts)
- [5. 参考](#5-参考)

<!-- /TOC -->

# 1. 搭建 Android 环境
按照 <https://github.com/riscv-android-src/riscv-android/blob/main/doc/android12.md> 的步骤，先搭建 android 开发环境。

# 2. 运行 android 模拟器
开启一个 tmux 终端，先把 android 模拟器跑起来：

```bash
$ emulator -no-qt -show-kernel -noaudio -selinux permissive -qemu -smp 1 -m 3584M -bios kernel/prebuilts/5.10/riscv64/fw_jump.bin
```
另外再开一个 tmux session ，测试一下， adb 是否能连接上模拟器：

```bash
$ adb devices
```

# 3. 编译 cts
在 ~/riscv-android-src 目录下，编译 cts，需要根据自身的 cpu ，配置 -j 后的参数：

```bash
$ m cts –j16
```

# 4. 跑 cts
区别于开发板中跑 cts ，在模拟器上跑 cts 不需要过多的复杂的步骤。
执行

```bash
$ cts-tradefed
```
在进入 cts 后，输入 `list d` 就可以看到当前所连接的模拟器了。

```bash
Serial         State    Allocation   Product  Variant  Build    Battery
emulator-5554  ONLINE   Unavailable  unknown  unknown  unknown  unknown
```
如果只需要运行默认的 CTS 测试用例或者某些免安装应用的 CTS 用例，就只需要执行指令

```bash
run cts
```
如果没有连接任何设备，CTS 会等到设备连接后再启动测试。如果连接了多台设备，则 CTS 会自动选择一台设备。如果需要指定某个设备跑cts的话，则增加参数： 

```bash
-s/--Serial deviceID
```

# 5. 参考

相关的 cts 指令可以通过 `run help` 查找，也可以参考该文档 <https://source.android.com/compatibility/cts/run> 和 <https://source.android.com/compatibility/cts/command-console-v2#ctsv2_reference>

在性能不佳的服务器上，有可能会出现不稳定的情况，这是因为 adb 掉线了。


![](./diagrams/android.png)

文章标题：**笔记：搭建 Cuttlefish 运行环境**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 有关 Cuttlefish](#2-有关-cuttlefish)
- [3. 操作系统](#3-操作系统)
- [4. 验证 KVM 可用性](#4-验证-kvm-可用性)
- [5. 安装 Cuttlefish 运行环境](#5-安装-cuttlefish-运行环境)
- [6. 下载 CuttleFish 镜像](#6-下载-cuttlefish-镜像)
- [7. 运行 Cuttlefish](#7-运行-cuttlefish)
	- [7.1. 启动 Cuttlefish](#71-启动-cuttlefish)
	- [7.2. 关闭 Cuttlefish](#72-关闭-cuttlefish)
- [8. 通过 adb 访问 Cuttlefish 虚拟设备](#8-通过-adb-访问-cuttlefish-虚拟设备)
- [9. 通过 Web 方式访问 Cuttlefish 虚拟设备](#9-通过-web-方式访问-cuttlefish-虚拟设备)

<!-- /TOC -->

# 1. 参考文档 

- [1] [Cuttlefish Virtual Android Devices][1] 官网对 Cuttlefish 的介绍
- [2] [Use Cuttlefish to Launch an AOSP Build][2] 官网介绍如何搭建 cuttlefish 运行环境
- [3] [The Android Cuttlefish emulator][3] 比较新 03/08/2022
- [4] [Building and using Cuttlefish][4] January 31, 2020
- [5] [Vmware Ubuntu20 Cuttlefish 模拟运行 Android12][5]

注意，Google 官方的 android 网站 URL 前缀凡是 <https://source.android.com/> 的国内大陆无法直接访问，可以替换为 <https://source.android.google.cn/>。

# 2. 有关 Cuttlefish

参考 [1]。

值得注意的是 Cuttlefish 以后有可能会取代 Goldfish。

# 3. 操作系统

我采用的是：

```bash
lsb_release -a
No LSB modules are available.
Distributor ID:	Ubuntu
Description:	Ubuntu 20.04.5 LTS
Release:	20.04
Codename:	focal
```

# 4. 验证 KVM 可用性

我一开始是想搭建一个 VirtualBox 虚拟机的，但发现即使在 VirtualBox 上打开了 Enabled VT-x/AMD-V 选项，launch cuttlefish 时依然会失败。所以我目前的建议是采用真实的物理机上安装 Ubuntu 的方式来运行 cuttlefish。

# 5. 安装 Cuttlefish 运行环境

基本上参考 [2] 中 “Launch Cuttlefish” 章节第 1 步的描述。国内情况下执行这些命令时需要有些调整，对调整，增加的命令，我嵌入了注释：

```bash
sudo apt install -y git devscripts config-package-dev debhelper-compat golang curl
# https 方式 clone 可能不通，换成 git/ssh 协议方式
git clone git@github.com:google/android-cuttlefish.git
cd android-cuttlefish
# for 循环执行 debuild 过程中间会执行 go build，需要提前设置 go 代理
go env -w GO111MODULE=on
go env -w GOPROXY=https://goproxy.cn,direct
for dir in base frontend; do
  cd $dir
  debuild -i -us -uc -b -d
  cd ..
done
sudo dpkg -i ./cuttlefish-base_*_*64.deb || sudo apt-get install -f
sudo dpkg -i ./cuttlefish-user_*_*64.deb || sudo apt-get install -f
sudo usermod -aG kvm,cvdnetwork,render $USER
sudo reboot
```

# 6. 下载 CuttleFish 镜像

参考 [2] 中 “Launch Cuttlefish” 章节第 2 步 ~ 第 7 步的描述，从 Android Continuous Integration 网站下载每日构建的镜像。

FIXME: 这个必须访问境外 Google 网站。我后面研究一下如何自己构建后再补充上来。

# 7. 运行 Cuttlefish

参考 [2] 中 “Launch Cuttlefish” 章节第 8 步

## 7.1. 启动 Cuttlefish

```bash
HOME=$PWD ./bin/launch_cvd --daemon
```

看到输出 

```bash
Virtual device booted successfully
VIRTUAL_DEVICE_BOOT_COMPLETED
```

说明 cuttlefish 成功运行起来了。

注意终端上打印的如下信息，指明了我们启动的虚拟设备名称为 cvd-1，以及该虚拟设备的一些运行日志的路径，譬如内核的日志和 launcher 的日志，还有 logcat。

```bash
The following files contain useful debugging information:
  Serial console is disabled; use -console=true to enable it.
  Kernel log: /home/u/ws/cf/cuttlefish/instances/cvd-1/kernel.log
  Logcat output: /home/u/ws/cf/cuttlefish/instances/cvd-1/logs/logcat
  Launcher log: /home/u/ws/cf/cuttlefish/instances/cvd-1/logs/launcher.log
  Instance configuration: /home/u/ws/cf/cuttlefish/instances/cvd-1/cuttlefish_config.json
  Instance environment: /home/u/ws/cf/.cuttlefish.sh
```

## 7.2. 关闭 Cuttlefish

```bash
HOME=$PWD ./bin/stop_cvd
stop_cvd I 01-11 10:08:15  7547  7547 main.cc:162] Successfully stopped device cvd-1: 0.0.0.0:6520
```

# 8. 通过 adb 访问 Cuttlefish 虚拟设备

adb 的操作和 goldfish 虚拟设备访问是一致的，譬如：

查看虚拟设备列表：

```bash
./bin/adb devices -l
List of devices attached
0.0.0.0:6520           device product:aosp_cf_x86_64_phone model:Cuttlefish_x86_64_phone device:vsoc_x86_64 transport_id:1
```

shell 登录：
```bash
./bin/adb shell
vsoc_x86_64:/ $ ls
acct        cache   data_mirror    init             metadata  oem          sdcard                  system       vendor_dlkm
apex        config  debug_ramdisk  init.environ.rc  mnt       postinstall  second_stage_resources  system_dlkm
bin         d       dev            linkerconfig     odm       proc         storage                 system_ext
bugreports  data    etc            lost+found       odm_dlkm  product      sys                     vendor
vsoc_x86_64:/ $ 
```

# 9. 通过 Web 方式访问 Cuttlefish 虚拟设备

cuttlefish 的最大特色就是可以通过浏览器访问，注意我发现目前 cuttlefish 比较挑浏览器，我测试发现可以正常工作的有 Chrome 或者 Edge，但是采用 Ubuntu 自带的 Firefox 却不行，这个问题当时困扰了我一段时间，一直以为是我环境搭建的问题，结果换了 Chrome/Edge 就好了。看来毕竟是 Google 自家的产品 :(。

cuttlefish 支持 webview 访问的端口是 8443。所以：

- 如果是本地访问，浏览器中输入 `https://192.168.2.6:8443/`。
- 如果是远程访问，我这里只尝试了局域网的访问，假设我所在的局域网网段是 "192.168.2.x"，cuttelfish 所在机器的 IP 是 "192.168.2.6"，则浏览器中输入 `https://192.168.2.6:8443/`。

![](./diagrams/20230111-cuttlefish-setup/cvd.png)



[1]:https://source.android.com/docs/setup/create/cuttlefish
[2]:https://source.android.com/docs/setup/create/cuttlefish-use
[3]:https://2net.co.uk/blog/cuttlefish-android12.html
[4]:https://nathanchance.dev/posts/building-using-cuttlefish/
[5]:https://coderfan.net/vmware-ubuntu20-cuttlefish-run-android12.html
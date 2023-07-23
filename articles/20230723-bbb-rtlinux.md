![](./diagrams/logo-linux.png)

文章标题：**工作笔记：基于 BBB 实验 RT-Linux**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>


<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 搭建过程描述](#2-搭建过程描述)
	- [2.1. 修改内核](#21-修改内核)
	- [2.2. 给内核打上 PREEMPT_RT 补丁。](#22-给内核打上-preempt_rt-补丁)
	- [2.3. 将 BBB-wireless 替换为 BBB](#23-将-bbb-wireless-替换为-bbb)
- [3. TFTP 方式动态加载内核启动](#3-tftp-方式动态加载内核启动)

<!-- /TOC -->


# 1. 参考文档 

- [参考 1] [Buildroot Training - Practical Labs][1]

最近尝试基于 BeagleBoneBlack 搭建 RT-Linux 的实验环境，将一些工作简单记录下来备忘。

以下所有改动都开源在 <https://gitee.com/unicornx/buildroot/tree/bootlin-rt/>。有问题欢迎提 issue/PR。

# 2. 搭建过程描述

主要前期的搭建流程基本上就是参考的 [参考 1][1]，基本上做到 "Root filesystem construction" 结束就差不多了，前面也就是练练手，后面就是自己定制化部分。

## 2.1. 修改内核

[参考 1][1] 里的 Linux 内核采用的是 5.15.35，可是我上 [PREEMPT_RT 补丁下载网站][2] 搜了一下，没有找到 5.15.35 对应的 rt 补丁，最近的找到的一个也是 5.15.36 的，所以我打算改一下内核。

这个很简单，利用 Buildroot，按照 [参考 1][1] 的 “Configuring Buildroot”，在原来的基础上修改 `BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE` 即可。

## 2.2. 给内核打上 PREEMPT_RT 补丁。

参考 ["Building a Real-time Linux Kernel in Ubuntu with PREEMPT_RT"][3] 这篇文章，只不过这篇文章是给 ubuntu 打补丁，而我们这里是给自己的内核打补丁。原理类似。

下载 rt 补丁：
```bash
wget https://cdn.kernel.org/pub/linux/kernel/projects/rt/5.15/older/patches-5.15.36-rt41.tar.xz
``````

解压：
```bash
xz -d patch-5.15.36-rt41.patch.xz
```

然后根据 [参考 1][1] 的 "Patch the Linux kernel" 的方法，将解压开的 patch 文件复制到 `board/bootlin/beagleboneblack/patches/linux/` 目录下，并 rename 为：`0003-patch-5.15.36-rt41.patch`。

剩下的就是按照 [参考 1][1] 将 patch 打上即可。

## 2.3. 将 BBB-wireless 替换为 BBB

[参考 1][1] 采用的板子是 BBB wireless，我的是普通的 BBB，所以为了使能我的 Ethernet 网口，我这里将 [参考 1][1] 中 设备树 dts 文件换成 `am335x-boneblack`，具体做法是类似 [参考 1][1] 修改 `BR2_LINUX_KERNEL_INTREE_DTS_NAME` 即可。如果按照 [参考 1][1] 的 "Use a rootfs overlay to setup the network" 使能了 USB network，最好将其回退掉，我发现这个改动似乎和 ethernet 口会有冲突。

另外需要注意的是，因为修改了 dts 文件的名字，我们在制作 SD-card 的时候不要忘记更改一下 boot 分区中的 `extlinux.conf` 文件的内容，主要是 dtb 文件的文件名，否则 SD-card 启动时会失败。

修改后如下：
```
label buildroot
  kernel /zImage
  devicetree /am335x-boneblack.dtb
  append console=ttyO0,115200 root=/dev/mmcblk0p2 rootwait
```

如果想启动就有 IP 地址，我这里采用了静态方式，具体方法是参考了 [参考 1][1] 中 "Use a rootfs overlay to setup the network" 里的 "IP address configuration" 的方式，修改了 `etc/network/interfaces` 这个文件如下：

```
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet static
address 192.168.2.10
netmask 255.255.255.0
gateway 192.168.2.1
```

# 3. TFTP 方式动态加载内核启动

每次改了内核要烧写 SD card 比较麻烦。我这里采用 TFTP 方式在启动阶段使用 U-boot 中的 TFTP 方式下载内核。

eMMC 中的 U-boot 我基本没动，就用出厂的版本即可，`2014.04-rc3` 够用了就行，如果实在要换也可以，但我嫌麻烦没有动。

主要修改是在 Host 侧，我这里用的是 Ubuntu 20.04， 具体做法如下：

安装 tftp 服务端：
```bash
sudo apt-get install tftp-hpa tftpd-hpa
```

创建 tftp 服务器工作目录:
```bash
cd $HOME/ws
mkdir -p tftp
chmod 777 tftp
```

配置 tftp 服务器:
```bash
sudo vi /etc/default/tftpd-hpa
```

修改文件如下，假设用于 tftp 的服务器端共享的主目录路径为 `/home/u/ws/tftp`。

```
# /etc/default/tftpd-hpa
TFTP_USERNAME="tftp"
TFTP_DIRECTORY="/home/u/ws/tftp"
TFTP_ADDRESS="[::]:69"
TFTP_OPTIONS="-l -c -s"
```

配置好后重启 tftp 服务即可：

```bash
sudo service tftpd-hpa restart
```

具体启动时，执行如下操作：

先将重新构建的 `zImage` 和 `am335x-boneblack.dtb` 文件拷贝到 `/home/u/ws/tftp` 下。

正常开机上电，eMMC 方式，进入 U-boot 后快速按任意键进入 u-boot 命令行界面，设置如下命令。假设 Host 主机 IP 为 192.168.2.14，Target 机（BBB）IP 地址为 192.168.2.10：

```bash
set tftp_boot 'echo Booting from tftp ...; set autoload no; setenv ipaddr 192.168.2.10; setenv serverip 192.168.2.14; tftp ${loadaddr} zImage; tftp ${fdtaddr} am335x-boneblack.dtb; setenv bootargs console=ttyO0,115200 root=/dev/mmcblk0p2 rootwait; bootz ${loadaddr} - ${fdtaddr}'
```

然后运行如下命令即可启动 TFTP 传输：
```bash
run tftp_boot
```

该命令会将 zImage 下载到 DDR 的 ${loadaddr} 处，将 am335x-boneblack.dtb 下载到 ${fdtaddr}，loadaddr 和 fdtaddr 是 U-boot 预先定义好的两个环境变量，分别是内核的加载地址（0x82000000）和 FDT 的加载地址（0x88000000）。下载完成功能后，继续设置内核启动参数 bootargs，最后调用 bootz 解压缩 zImage 即可启动内核。


[1]:https://bootlin.com/doc/training/buildroot/buildroot-labs.pdf
[2]:https://cdn.kernel.org/pub/linux/kernel/projects/rt/
[3]:https://www.acontis.com/en/building-a-real-time-linux-kernel-in-ubuntu-preemptrt.html
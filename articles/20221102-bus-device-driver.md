![](./diagrams/logo-linux.png)

文章标题：**Linux 驱动模型之三剑客**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

说得好听叫三剑客，其实就是 Bus、Device 和 Driver。简单做个笔记，方便回顾时快速复习。

本文暂时不涉及内核中 Bus/Device/Driver 的详细实现，需要再补充吧，感觉这部分内容比较成熟，网上资料也不少了。

文章大纲

<!-- TOC -->

- [1. 参考：](#1-参考)
- [2. 三剑客极简笔记](#2-三剑客极简笔记)

<!-- /TOC -->

# 1. 参考：

- [Kernel doc - Driver Model](https://docs.kernel.org/driver-api/driver-model/index.html)
- 本文代码基于 Linux 内核代码 v6.0
- [Linux设备模型(6)_Bus](http://www.wowotech.net/device_model/bus.html), 注意例子代码对应的内核版本有点旧了，但原理基本未变
- [Linux设备模型(5)_device和device driver](http://www.wowotech.net/device_model/device_and_driver.html)，注意例子代码对应的内核版本有点旧了，但原理基本未变

# 2. 三剑客极简笔记

- Device

  [`struct device`](https://elixir.bootlin.com/linux/v6.0/source/include/linux/device.h#L555)

- Driver

  [`struct device_driver`](https://elixir.bootlin.com/linux/v6.0/source/include/linux/device/driver.h#L96)

- Bus

  [`struct bus_type`](https://elixir.bootlin.com/linux/v6.0/source/include/linux/device/bus.h#L84)


对比上面的三个结构体，你会发现：总线中既定义了设备，也定义了驱动；设备中既有总线，也有驱动；驱动中既有总线也有设备相关的信息。那这三个的关系到底是什么呢？

三者的关系是：

每次系统中添加一个设备，都要向总线注册（通过调用相关的 register 函数），注册完成的设备会被内核维护在一个链表中。每次系统中添加一个驱动，也要向总线注册（通过调用相关的 register 函数），注册完成的驱动也会被内核维护在一个链表中。

添加设备的过程中，内核会搜索驱动链表中是否有驱动和该设备匹配，如果匹配则会为每一个设备建立一个 `struct device` 对象并触发 probe 回调函数（我们也常常将这个过程称之为设备的枚举），如果匹配不到驱动就等待。需要注意的是，内核并不是直接将 `struct device` 结构直接暴露给驱动程序，而是会在上面封装（类似 OOP 中的派生）新的设备类型再通过 probe 回调传给驱动代码。譬如 `struct pci_dev`/`struct usb_interface`/`struct platform_device`, 所以编写驱动代码时我们并不会直接面对 `struct device`。类似地，移除设备的过程中会触发 remove 等回调函数，然后销毁 `struct device` 对象；

添加驱动的过程中，会搜索设备链表中是否有设备和该驱动匹配，如果匹配同样会走上面类似的设备枚举操作。删除驱动的过程中，也会执行相反的操作，并可能导致设备和驱动解绑。

注册设备的机会：
- 系统启动初始化时，内核会扫描连接了哪些设备，特别是对于那些不支持热插拔的设备，主要是在系统启动阶段完成设备的注册。
- 针对支持热插拔的设备，则在设备插入总线时完成设备的注册。

注册驱动的机会：
- 驱动模块被加载到内核中的时候会触发驱动的注册过程。可能是在内核引导阶段（对于 builtin 的驱动模块），或者是在进入用户态后的 init 阶段（加载 `/lib/modules` 下的 ko 文件），也或者是当我们手动 insmod 加载 ko 文件的时候。

probe 动作实际是由 Bus 负责触发，这不难理解，Device 和 Driver 都是挂载在 Bus 上，因此只有 Bus 最清楚应该为哪些 Device、哪些 Driver 进行配对。

每个 Bus 都有一个 `drivers_autoprobe` 变量，用于控制是否在 Device 或者 Driver 注册时自动 probe。该变量默认为 1（即自动 probe），Bus 将它开放到 sysfs 中了，因而可在用户空间修改，进而控制 probe 的行为。



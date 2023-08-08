![](./diagrams/RTLinux.png)

文章标题：**笔记：Cyclictest 工作原理分析**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

本笔记简单总结了 RTLinux 测试中常用的 cyclictest 的安装方法，并简单分析了 cyclictest 的工作原理。

cyclictest 代码部分基于 rt-tests v2.3。

<!-- TOC -->

- [1. 什么是 cyclictest](#1-什么是-cyclictest)
- [2. 编译 cyclictest](#2-编译-cyclictest)
- [3. cyclictest 代码分析](#3-cyclictest-代码分析)
	- [3.1. "抢占延迟" 的概念](#31-抢占延迟-的概念)
	- [3.2. cyclictest 计算延迟算法分析](#32-cyclictest-计算延迟算法分析)

<!-- /TOC -->

# 1. 什么是 cyclictest

参考 [Linux 基金会上 Cyclictest 主页][2] 的定义，摘录如下：

> Cyclictest accurately and repeatedly measures the difference between a thread's intended wake-up time and the time at which it actually wakes up in order to provide statistics about the system's latencies. It can measure latencies in real-time systems caused by the hardware, the firmware, and the operating system.
>
> The original test was written by Thomas Gleixner (tglx), but several people have subsequently contributed modifications. Cyclictest is currently maintained by Clark Williams and John Kacur and is part of the test suite [rt-tests][1].

# 2. 编译 cyclictest

因为 cyclictest 现在是 [rt-tests][1] 软件包的一部分，所以我们直接编译 rt-tests 软件包即可得到 cyclictest。

首先安装依赖，我的构建环境是 Ubuntu。

```bash
sudo apt update
sudo apt install build-essential libnuma-dev
```

下载源码仓库：

```bash
git clone git://git.kernel.org/pub/scm/utils/rt-tests/rt-tests.git
cd 
```

我这里用的是 2.3 的版本
```bash
cd rt-tests
git checkout v2.3
```

现在就可以编译了，如果是本地编译：
```bash
make
```

如果是交叉编译，可以在 make 前指定 `CROSS_COMPILE`。

编译后 cyclictest 就生成在 rt-tests 目录下，同时也包括了其他的 rt-tests 中的工具软件。

```bash
$ ls -l
total 1924
drwxrwxr-x  2 u u   4096 8月   8 14:27 bld
-rw-rw-r--  1 u u  17987 8月   8 14:01 COPYING
-rwxrwxr-x  1 u u 160976 8月   8 14:27 cyclicdeadline
-rwxrwxr-x  1 u u 228952 8月   8 14:27 cyclictest
-rwxrwxr-x  1 u u 179848 8月   8 14:27 deadline_test
lrwxrwxrwx  1 u u     41 8月   8 14:27 get_cyclictest_snapshot -> src/cyclictest/get_cyclictest_snapshot.py
-rwxrwxr-x  1 u u  53840 8月   8 14:27 hackbench
lrwxrwxrwx  1 u u     30 8月   8 14:27 hwlatdetect -> src/hwlatdetect/hwlatdetect.py
-rw-rw-r--  1 u u    159 8月   8 14:01 MAINTAINERS
-rw-rw-r--  1 u u   7754 8月   8 14:05 Makefile
-rwxrwxr-x  1 u u 136256 8月   8 14:27 oslat
-rwxrwxr-x  1 u u  97864 8月   8 14:27 pip_stress
-rwxrwxr-x  1 u u 161296 8月   8 14:27 pi_stress
-rwxrwxr-x  1 u u 125312 8月   8 14:27 pmqtest
-rwxrwxr-x  1 u u 114120 8月   8 14:27 ptsematest
-rwxrwxr-x  1 u u  55488 8月   8 14:27 queuelat
-rw-rw-r--  1 u u   4013 8月   8 14:01 README.markdown
-rwxrwxr-x  1 u u 119640 8月   8 14:27 rt-migrate-test
-rwxrwxr-x  1 u u 120872 8月   8 14:27 signaltest
-rwxrwxr-x  1 u u 122704 8月   8 14:27 sigwaittest
drwxrwxr-x 20 u u   4096 8月   8 14:01 src
-rwxrwxr-x  1 u u  94304 8月   8 14:27 ssdd
-rwxrwxr-x  1 u u 125320 8月   8 14:27 svsematest
```

# 3. cyclictest 代码分析

## 3.1. "抢占延迟" 的概念

详细分析可以参考以前的一篇笔记：[《实时 Linux（Real-Time Linux）》][3] 中有关 "抢占延迟（preemption latency）" 介绍。cyclictest 计算的就是这个。“抢占延迟”时间的长短反映了一个具体的实时任务的响应速度，即该处于等待中的实时任务从外部信号开始触发起到最后该实时任务抢占其他运行态任务获取处理器进而开始执行自己的处理逻辑为止之间花费的时间。这个响应速度是我们衡量一个实时应用，也是进而一个实时系统整体的所谓实时性的主要关键性能指标。

## 3.2. cyclictest 计算延迟算法分析

Cyclictest 原来是设计用于测量高精度定时器的性能，但同时根据我们上一节所介绍的 “抢占延迟” 的定义同样可以用其来测量我们关心的 “抢占延迟” 值。

Cyclictest 创建实时任务（线程）运行测试代码，实时任务执行 `timerthread()` 函数，该函数的关键逻辑的伪代码描述如下：

```cpp
......
clock_gettime(......, &now1);           // (1)
next = now1; next += interval;          // (2)
clock_nanosleep(......, &next, ......); // (3)
clock_gettime(......, &now2);           // (4)
diff = calcdiff(now2, next);            // (5)
......
```

- 第 (1) 步：调用系统调用函数 `clock_gettime()` 以纳秒级别的精度获取当前绝对时间并记为 `now1`。注意运行时也可以选择调用其他的获取系统时间函数，这取决于运行 cyclitest 程序时所指定的命令行参数。

- 第 (2) 步：在第 (3) 步执行睡眠之前先计算得到期望该任务被唤醒的绝对时间（expected time）并记为 `next`。计算的公式是：`next = now1 + interval`。其中 `now1` 是第 (1) 步得到的当前绝对时间，`interval` 是期望当前任务睡眠的时长。

- 第 (3) 步：调用系统调用函数 `clock_nanosleep()` 以纳秒级别的精度将本任务挂起并等待一个定时器唤醒，定时器的期望被唤醒的绝对时间设置为 `next` 的值。注意运行时也可以选择调用其他类型的睡眠函数，这取决于运行 cyclitest 程序时所指定的命令行参数。

- 第 (4) 步：本任务睡眠后，间隔 `intervel` 的时间后定时器会到期，并在我们期望的时间点 `next` 上触发内核执行抢占动作，本质上是激活定时器中断触发外部输入信号（External input signal），然后经历上一章 “抢占延迟” 所定义的内部处理过程。由于定时器到期的任务是实时任务，优先级较高，可以抢占当前处理器上的其他任务而再次获得处理器的使用权并继续运行进入用户态执行第 (4) 步，即调用系统调用函数 `clock_gettime()` 以纳秒级别的精度再次获取当前绝对时间并记为 `now2`。注意这里的抢占可以包括内核态抢占和用户态抢占，这取决于第 (4) 步本任务抢占处理器时被抢占任务运行在何种状态下（用户态还是内核态），所以测试时可以通过增加负载，多运行一些任务以覆盖更全面的抢占类型测试。

- 第 (5) 步：`calcdiff()/calcdiff_ns()`，该函数可以用来计算两个绝对时间之间间隔的时间，即计算`“diff = now2 – next”`，该 `diff` 值即我们所关心的 “抢占延迟” 值。具体参考下图：

```
                     (Preemption latency)    
      -- interval --  ----- diff ------
     /              \/                 \     time
-----+---------------+-----------------+------->
     ^               ^                 ^
     |               |                 |
     |               |                 |
    now1            next               now2
```

[1]:https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/rt-tests
[2]:https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/cyclictest/start
[3]:./20230727-rt-linux.md

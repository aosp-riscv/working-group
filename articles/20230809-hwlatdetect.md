![](./diagrams/RTLinux.png)

文章标题：**笔记：hwlatdetect 介绍**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

本笔记简单总结了 RTLinux 测试中常用的 hwlatdetect 的安装方法，并简单介绍了 hwlatdetect 的使用。

hwlatdetect 代码部分基于 rt-tests v2.3。

Linux 内核部分代码基于版本 v6.1.38

<!-- TOC -->

- [1. 什么是 hwlatdetect](#1-什么是-hwlatdetect)
- [2. 编译 hwlatdetect](#2-编译-hwlatdetect)
- [3. 理解 hwlatdetect](#3-理解-hwlatdetect)
- [4. 使用 hwlatdetect](#4-使用-hwlatdetect)

<!-- /TOC -->

# 1. 什么是 hwlatdetect

参考 rt-tests 源码中自带的 man 手册页：`src/hwlatdetect/hwlatdetect.8`

> hwlatdetect is a program that controls the ftrace kernel hardware latency detector (hwlatdetector).hwlatdetector is used to detect large system latencies induced by the behavior of certain underlying hardware or firmware, independent of Linux itself. 

这里说了几个意思：

- hwlatdetect 是 hardware latency detector 的缩写

- hwlatdetect 本身是 [rt-tests][1] 中自带的一个程序，仔细观察会发现实际上只是提供了一个 python 脚本 `src/hwlatdetect/hwlatdetect.py`

- rt-tests 中提供的 hwlatdetect 本质上只是一个 python 脚本，内部实际上是封装 Linux 内核的一个同名的叫做 hwlat_detector 的 tracer。所以如果要启用 rt-tests 中的 hwlatdtect，我们需要在构建 Linux 内核时使能 `CONFIG_HWLAT_TRACER`。有关 Linux 中这个 hwlat detector tracer 的说明，可以看一下 [内核文档 Hardware Latency Detector][2]。需要注意的是，在早期，内核部分的支持是独立于 Linux，以一个 kernel module 的方式实现在 `PREEMPT_RT` 补丁中的，但后来这部分功能被 Linux 内核吸收了，实现为一个 tracer。具体可以查一下 Linux 中 `kernel/trace/trace_hwlat.c` 这个文件修改的历史，最早大概是在 4.9 合入主线的（e7c15cd8a113335cf7154f027c9c8da1a92238ee）。

- 那么这个 hwlatdetect 究竟有什么用呢？我的理解是，我们可以用这个工具检查一下底层的硬件上是否存在一些特别的特性导致我们在这个硬件平台上运行 RT-Linux 时会遇到特别大的延迟问题。man 手册中提到了 x86 上的 SMI 中断，这也是 hwlatdetect 最早开发的原因，用于检测 x86/amd 上的 SMI 中断对系统延迟的影响。但目前的实现已经和具体 ARCH 无关了，所以我们也可以用这个工具来测试其他的 ARCH 平台。

# 2. 编译 hwlatdetect

参考 [《笔记：Cyclictest 工作原理分析》][3]。通过下载 rt-tests 软件包即可获得 `hwlatdetect.py` 脚本。 make 过后会在 rt-tests 源码目录下生成一个符号链接 `hwlatdetect`，实际还是指向 `src/hwlatdetect/hwlatdetect.py`。

# 3. 理解 hwlatdetect

因为 rt-tests 中的 hwlatdetect 只不过是 linux 内核中 hwlat_detector tracer 的封装，所以直接去看内核里的实现就好了。

我觉得有关 hwlatdetect 实现机制，参考资料看下面几个就好了：

- [内核文档 Hardware Latency Detector][2]，摘录如下：

  > The hardware latency detector works by hogging one of the cpus for configurable amounts of time (with interrupts disabled), polling the CPU Time Stamp Counter for some period, then looking for gaps in the TSC data. Any gap indicates a time when the polling was interrupted and since the interrupts are disabled, the only thing that could do that would be an SMI or other hardware hiccup (or an NMI, but those can be tracked).

- [内核文档 OSNOISE Tracer][5] 中也有一段对 hwlatdetect 的描述，写得不错，摘录如下：

  > hwlat_detector is one of the tools used to identify the most complex
  > source of noise: *hardware noise*.
  > 
  > In a nutshell, the hwlat_detector creates a thread that runs
  > periodically for a given period. At the beginning of a period, the thread
  > disables interrupt and starts sampling. While running, the hwlatd
  > thread reads the time in a loop. As interrupts are disabled, threads,
  > IRQs, and SoftIRQs cannot interfere with the hwlatd thread. Hence, the
  > cause of any gap between two different reads of the time roots either on
  > NMI or in the hardware itself. At the end of the period, hwlatd enables
  > interrupts and reports the max observed gap between the reads. It also
  > prints a NMI occurrence counter. If the output does not report NMI
  > executions, the user can conclude that the hardware is the culprit for
  > the latency. The hwlat detects the NMI execution by observing
  > the entry and exit of a NMI.


- Linux 中 `kernel/trace/trace_hwlat.c` 这个文件最早提交的 commitid e7c15cd8a113335cf7154f027c9c8da1a92238ee 对应的 commitment 的注释, 摘录一下：

  >    The logic is pretty simple. It simply creates a thread that spins on a
  >    single CPU for a specified amount of time (width) within a periodic window
  >    (window). These numbers may be adjusted by their cooresponding names in
  >    
  >       /sys/kernel/tracing/hwlat_detector/
  >    
  >    The defaults are window = 1000000 us (1 second)
  >                     width  =  500000 us (1/2 second)
  >    
  >    The loop consists of:
  >    
  >            t1 = trace_clock_local();
  >            t2 = trace_clock_local();
  >    
  >    Where trace_clock_local() is a variant of sched_clock().
  >    
  >    The difference of t2 - t1 is recorded as the "inner" timestamp and also the
  >    timestamp  t1 - prev_t2 is recorded as the "outer" timestamp. If either of
  >    these differences are greater than the time denoted in
  >    /sys/kernel/tracing/tracing_thresh then it records the event.
  >    
  >    When this tracer is started, and tracing_thresh is zero, it changes to the
  >    default threshold of 10 us.

- Linux 中 `HWLAT_TRACER` 的 help 帮助：`kernel/trace/Kconfig`

  >	 This tracer, when enabled will create one or more kernel threads,
  >	 depending on what the cpumask file is set to, which each thread
  >	 spinning in a loop looking for interruptions caused by
  >	 something other than the kernel. For example, if a
  >	 System Management Interrupt (SMI) takes a noticeable amount of
  >	 time, this tracer will detect it. This is useful for testing
  >	 if a system is reliable for Real Time tasks.
  >
  >	 Some files are created in the tracing directory when this
  >	 is enabled:
  >
  >	   hwlat_detector/width   - time in usecs for how long to spin for
  >	   hwlat_detector/window  - time in usecs between the start of each
  >				     iteration
  >
  >	 A kernel thread is created that will spin with interrupts disabled
  >	 for "width" microseconds in every "window" cycle. It will not spin
  >	 for "window - width" microseconds, where the system can
  >	 continue to operate.
  >
  >	 The output will appear in the trace and trace_pipe files.
  >
  >	 When the tracer is not running, it has no affect on the system,
  >	 but when it is running, it can cause the system to be
  >	 periodically non responsive. Do not run this tracer on a
  >	 production system.
  >
  >	 To enable this tracer, echo in "hwlat" into the current_tracer
  >	 file. Every time a latency is greater than tracing_thresh, it will
  >	 be recorded into the ring buffer.

差不多就是这个意思，执行 detection 时，CPU 上会关闭所有中断(不包括 NMI)，然后执行一个内核线程，通过 looping 方式周期性查询 Time Stamp Counter, 判断两次读取之间的时间差是否出现大的延迟，因为关中断，所以任务调度、其他硬中断和软中断都不会干扰（抢占） hwlatd 线程，如果出现并且大于你的期望值的延迟则说明存在 NMI 或者硬件系统上有影响实时性的问题。

# 4. 使用 hwlatdetect

具体的使用很简单，我指的是基于 rt-tests 的 hwlatdetect 脚本封装，自己手动操作 linux 的 tracer 不在这里讨论。可以参考 Redhat 提供的一份使用说明：[Chapter 3. Running and interpreting hardware and firmware latency tests][4]。摘录如下：

> Run hwlatdetect, specifying the test duration in seconds.
> hwlatdetect looks for hardware and firmware-induced latencies by polling the clock-source and looking for unexplained gaps.

```bash
# hwlatdetect --duration=60s
hwlatdetect:  test duration 60 seconds
	detector: tracer
	parameters:
		Latency threshold:    10us
		Sample window:        1000000us
		Sample width:         500000us
		Non-sampling period:  500000us
		Output File:          None

Starting test
test finished
Max Latency: Below threshold
Samples recorded: 0
Samples exceeding threshold: 0
```


[1]:https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/rt-tests
[2]:https://elixir.bootlin.com/linux/v6.1.38/source/Documentation/trace/hwlat_detector.rst
[3]:./20230808-cyclictest.md
[4]:https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux_for_real_time/8/html/optimizing_rhel_8_for_real_time_for_low_latency_operation/assembly_running-and-interpreting-hardware-and-firmware-latency-tests_optimizing-rhel8-for-real-time-for-low-latency-operation
[5]:https://elixir.bootlin.com/linux/v6.1.38/source/Documentation/trace/osnoise-tracer.rst

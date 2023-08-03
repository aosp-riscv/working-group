![](./diagrams/RTLinux.png)

文章标题：**笔记：Linux PREEMPT_RT 补丁分析报告**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

本笔记总结了 Linux 内核的 `PREEMPT_RT` 抢占模型，分析了该抢占类型中抢占动作的实现机制。

代码部分基于 Linux 5.15.36；涉及架构时以 ARM 为例子。

感觉这块内容有点大，还有很多内容没来得及消化，先写这么多，后面再慢慢补充。

<!-- TOC -->

- [1. `PREEMPT_RT` 抢占模式](#1-preempt_rt-抢占模式)
- [2. 针对硬中断的实时化改造](#2-针对硬中断的实时化改造)
	- [2.1. 硬中断中存在的不确定性问题](#21-硬中断中存在的不确定性问题)
	- [2.2. 改进方法](#22-改进方法)
- [3. 针对软中断的实时化改造](#3-针对软中断的实时化改造)
	- [3.1. 软中断的不确定性](#31-软中断的不确定性)
	- [3.2. 改进方法](#32-改进方法)
- [4. 针对锁同步机制的实时化改造](#4-针对锁同步机制的实时化改造)
	- [4.1. 锁同步机制存在的不确定性问题](#41-锁同步机制存在的不确定性问题)
	- [4.2. 改进方法](#42-改进方法)
- [5. 参考文档](#5-参考文档)

<!-- /TOC -->

在 [《笔记：Linux 内核的抢占模型》][1] 中我们分析总结了 Linux 主线内核的抢占模式。从中我们可以看到，`PREEMPT_NONE` 只具备用户态抢占能力，`PREEMPT_VOLUNTARY` 可以认为是对 `PREEMPT_NONE` 的有益补充，但由于不具备内核抢占能力所以一旦在内核态下执行路径较长则必然会产生大延迟，对于实时应用来说是很大的问题。

`PREEMPT` 模式引入了内核抢占，这样内核可以在除了临界区之外的所有地方都可以进行有效抢占了。打开 `PREEMPT` 内核抢占模式后可以提供 ms 级别的实时应用的支持，离一个真正的 RTOS 又近了一步，但仍然还有一些 “确定性” 问题需要解决。现在本文中将一些主要的问题分析总结一下。

# 1. `PREEMPT_RT` 抢占模式

`PREEMPT_RT` 抢占模式实现了 "Fully Preemptible Kernel (Real-Time)"，可以说是专门为了实现 Linux 对 RTOS 的支持而增加的一种抢占模式。由于 Linux 一开始主要面向的是桌面和服务器环境，所以为了支持 Real-Time，需要修改的地方很多。用于为 Linux 添加 `PREEMPT_RT` 的修改我们也常称之为 `PREEMPT_RT` 补丁。更多有关 Linux `PREEMPT_RT` 补丁 的介绍可以参考另外一篇笔记 [《实时 Linux（Real-Time Linux）》][3]。

`PREEMPT_RT` 补丁的历史源远流长，最早可以上溯至 2004 年，当时的几位杰出工程师：Ingo Molnar、Thomas Gleixner 和 Steven Rostedt 搜集整理了前期众多开发人员对 Linux 的实时化补丁，整合出了我们已知的 `PREEMPT_RT` 补丁并开始了漫长的 upstream 工作，期间几度坎坷，虽然还未完全合入，但很大一部分有价值的工作已经被内核主线所吸收，对提高内核的执行效率起到了很大的帮助。这些工作包括以下但不局限于这些:
- Generic Timekeeping
- High resolution timers
- Mutex infrastructure
- Generic interrupt handling infrastructure
- Priority inheritance for user space mutexes
- Preemptible and hierarchical RCU
- Threaded interrupt handlers
- Tracing infrastructure
- Lock dependency validator
- …

可喜的是，通过大家不懈的努力和 Linux 社区的赞助，特别是 Thomas Gleixner 先生的坚持下，`PREEMPT_RT` 补丁距离完全合入内核主线已经不是一件遥不可及的事情。[5.3 内核中正式加入了 `CONFIG_PREEMPT_RT` 这个配置项][4]，这标志了 `PREEMPT_RT` 补丁正式开启了合入内核主线工作，此后的合并工作开始加速，截至本文写作对应的 6.4 内核，虽然该项工作还未正式完成，但剩余的未合入的 `PREEMPT_RT` 补丁已经很少了，相信很快这项工作会完成，实在是太不容易了，快 20 年了，应该算得上是 Linux 有史以来历时最漫长的一个补丁提交。

`PREEMPT_RT` 差不多可以看成是 `PREEMPT` 的超集，（参考下面代码，摘录自 `kernel/Kconfig.preempt`）。

```
config PREEMPT_RT
	bool "Fully Preemptible Kernel (Real-Time)"
	depends on EXPERT && ARCH_SUPPORTS_RT
	select PREEMPTION
	help
	  This option turns the kernel into a real-time kernel by replacing
	  various locking primitives (spinlocks, rwlocks, etc.) with
	  preemptible priority-inheritance aware variants, enforcing
	  interrupt threading and introducing mechanisms to break up long
	  non-preemptible sections. This makes the kernel, except for very
	  low level and critical code pathes (entry code, scheduler, low
	  level interrupt handling) fully preemptible and brings most
	  execution contexts under scheduler control.

	  Select this if you are building a kernel for systems which
	  require real-time guarantees.
```

给主线内核打上 `PREEMPT_RT` 补丁后缺省的抢占模式依然是 `PREEMPT_NONE`，编译实时 Linux 时需要手动选择 `PREEMPT_RT`，使能后的 `PREEMPT_RT` 除了包含中断强制线程化（`IRQ_FORCED_THREADING`）以及 `PREEMPT` 所支持的内核抢占机制之外，其他核心的修改主要体现在以下项目中。

# 2. 针对硬中断的实时化改造

## 2.1. 硬中断中存在的不确定性问题

默认情况下主线内核的中断处理采用了中断延迟技术，将整个中断处理过程分为上半部（top half，也称为硬中断，“hard” irq）和下半部（button half）。其中在上半部处理过程中关中断，在下半部处理中开中断，这么做是为了解决在中断处理中工作量和工作速度之间的冲突矛盾。具体来说，硬中断处理具有最高抢占优先级，硬中断发生后，处理器会在硬件层面打断当前处理流程，优先处理中断；同时内核为了避免复杂的中断嵌套，强制在关中断情况下执行上半部，其中最主要的就是调用中断服务程序（ISR，即通过 `request_irq` 注册的中断处理程序），所以内核编程时要求在 ISR 中尽量执行快速而短小的任务，因为从整体系统性能考虑出发，关闭中断后相当于禁止了任何类型的抢占，其他任务和硬件的请求都得不到响应，最终导致系统任务阻塞时间过长，无法及时对其他请求做出反应。而中断下半部是在稍后更加安全的时间内被中断上半部调度执行的例程，当执行中断下半部时，中断是打开的，我们可以在下半部中安排处理更多的大量的工作任务而不用担心阻塞其他的任务。

从上面的执行流程可以看出，虽然内核引入了上下半部的机制尽量避免在整个硬中断处理中的大延迟。但就上半部过程来说，由于 ISR 的执行时间不可控，仍然有可能引入长时间的处理，这和实时系统要求的 “确定性” 是背道而驰的。

## 2.2. 改进方法

为了解决主线内核中的硬中断存在的不确定性问题，内核从 2.6.30 版本开始引入了中断线程化的概念, 2.6.39 引入 threadirqs & `CONFIG_IRQ_FORCED_THREADING`` & `IRQF_NO_THREAD`，同时可以配置指定将 softirq 也进行线程化。

中断线程化的核心思想其实很简单，就是尽可能地将 ISR 中的计算任务放到内核线程中去完成（原则上可以将中断处理的所有工作全部放到线程中去完成，但也有例外，譬如对于共享中断线的一些处理还需要在硬中断上下文中完成）。中断线程化后带来的好处很多，最突出的两点就是：

- 不像原来框架下一个有问题的 ISR 可能会永久阻塞系统，逻辑全部进入线程化的下半部了，避免了原来上半部和下半部之间可能会发生的相互死锁现象。
- 中断任务和其他任务一样可以拥有自己的优先级，一起参与 “可受内核控制下” 的优先级调度，这么做也满足了实时系统所谓 “可预期” 性目标，经过用户的仔细设计，在确保不影响中断处理的前提下，高优先级的任务就可以抢占中断任务。

内核新提供了一个 API 函数 `request_threaded_irq()` 可以用于申请一个线程化的 IRQ。函数原型如下：

```cpp
int request_threaded_irq(unsigned int irq, irq_handler_t handler,
			 irq_handler_t thread_fn, unsigned long irqflags,
			 const char *devname, void *dev_id)
```

如果 `thread_fn`` 这个回调函数指针不为 NULL，kernel就会创建一个名字为 “irq/%d-%s” 的线程，优先级为50（`MAX_RT_PRIO / 2`），`%d` 对应着中断号 irq，`%s` 对应着设备名name。中断的上半部（硬中断）执行 handler（一般直接赋值为 NULL，除非要处理共享中断设备问题），在做完必要的处理工作之后，返回 `IRQ_WAKE_THREAD`` 之后 kernel 会唤醒 “ irq/%d-%s” 线程执行 `thread_fn` 函数以下半部的形式运行。如果不希望线程化，可以在 flags 中指定 `IRQF_NO_THREAD` 或者 `IRQF_TIMER`，譬如定时器（timer）中断在`PREEMPT_RT` 补丁中就没有线程化。

在主线内核版本中，为了保持和以往版本的兼容性，用于申请中断号的函数 `request_irq()` 虽然内部被替换为直接调用 `request_threaded_irq()`，但缺省情况下并没有启动线程化，第三个参数传入的是 NULL。参考如下代码：

```cpp
static inline int __must_check
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
	return request_threaded_irq(irq, handler, NULL, flags, name, dev);
}
```

具体的调用路径是：`request_threaded_irq()` -> `__setup_irq()` -> `setup_irq_thread()`。

```cpp
static int
__setup_irq(unsigned int irq, struct irq_desc *desc, struct irqaction *new)
{
.....
		if (irq_settings_can_thread(desc)) {
			ret = irq_setup_forced_threading(new);
......
	/*
	 * Create a handler thread when a thread function is supplied
	 * and the interrupt does not nest into another interrupt
	 * thread.
	 */
	if (new->thread_fn && !nested) {
		ret = setup_irq_thread(new, irq, false);
		if (ret)
			goto out_mput;
		if (new->secondary) {
			ret = setup_irq_thread(new->secondary, irq, true);
			if (ret)
				goto out_thread;
		}
	}
}

static int
setup_irq_thread(struct irqaction *new, unsigned int irq, bool secondary)
{
	struct task_struct *t;

	if (!secondary) {
		t = kthread_create(irq_thread, new, "irq/%d-%s", irq,
				   new->name);
	} else {
		t = kthread_create(irq_thread, new, "irq/%d-s-%s", irq,
				   new->name);
	}

	if (IS_ERR(t))
		return PTR_ERR(t);

	sched_set_fifo(t);	
......
}
```

```cpp
void sched_set_fifo(struct task_struct *p)
{
	struct sched_param sp = { .sched_priority = MAX_RT_PRIO / 2 };
	WARN_ON_ONCE(sched_setscheduler_nocheck(p, SCHED_FIFO, &sp) != 0);
}
EXPORT_SYMBOL_GPL(sched_set_fifo);
```

在 `setup_irq_thread()` 函数中会创建 kernel thread 并通过 `sched_set_fifo()` 将该任务的调度策略设置为 `SCHED_FIFO`,优先级设置为 50；其传入的线程执行函数 `irq_thread()` 会根据 `force_irqthreads()` 判断。而 `force_irqthreads()` 最终由 `IRQ_FORCED_THREADING` 选项控制是否要对没有线程化 IRQ 的情况进行强制线程化。

```cpp
#ifdef CONFIG_IRQ_FORCED_THREADING
# ifdef CONFIG_PREEMPT_RT
#  define force_irqthreads()	(true)
# else
DECLARE_STATIC_KEY_FALSE(force_irqthreads_key);
#  define force_irqthreads()	(static_branch_unlikely(&force_irqthreads_key))
# endif
#else
#define force_irqthreads()	(false)
#endif

/*
 * Interrupt handler thread
 */
static int irq_thread(void *data)
{
......
	irqreturn_t (*handler_fn)(struct irq_desc *desc,
			struct irqaction *action);

	if (force_irqthreads() && test_bit(IRQTF_FORCED_THREAD,
					   &action->thread_flags))
		handler_fn = irq_forced_thread_fn;
	else
		handler_fn = irq_thread_fn;
......
}
```

早期主线内核中缺省情况下 `IRQ_FORCED_THREADING` 这个配置并没有使能，所以在当时版本中大量的中断实际上并没有被线程化。中断上半部引入的延迟一定是存在的。从 3.12 开始，ARM 开始 “Allow forced irq threading”，`IRQ_FORCED_THREADING` 被加入 `arch/arm/Kconfig`；4.2 中 arm64 也默认使能了 `IRQ_FORCED_THREADING`。从 5.4 开始，也就是在 5.3 开启 PREEMPT_RT 的主线化工作后，内核 “Force interrupt threading on RT”（commitid：b6a32bbd8735def2d0d696ba59205d1874b7800f），具体代码参考上面 `force_irqthreads()` 的定义部分，所以现在对于主线内核（无需额外打上 `PREEMPT_RT`` 补丁）只要我们在配置中选择了 `PREEMPT_RT` 抢占模式，则对于 ARM/ARM64, 中断强制线程化将默认被使能，我们也无需通过启动内核时通过命令行参数进行额外的指定了。

# 3. 针对软中断的实时化改造

## 3.1. 软中断的不确定性

软中断（softirq）是在 Linux 2.3 版本期间从原始的 BH 机制（现在已经被废弃）衍生出的一种中断延迟处理技术机制，当时设计 softirq 主要是预留给对时间要求比较严格的底半部使用，上半部触发软中断后内核会 **尽可能地** 安排由触发软中断的那个 CPU 负责执行它所触发的软中断处理函数，也就是说这种情况下软中断会在中断上下文中立即得到执行，和硬中断的唯一区别仅仅是软中断的执行上下文是在开中断的条件下。注意这里说 **尽可能地** 含义是指内核如果发现同一个 CPU 上软中断来得过于频繁，譬如在当前软中断执行过程中频繁触发硬中断且这些硬中断会频繁 raise 软中断，那么内核会直接将后续的软中断处理延迟到 per-CPU 的内核 ksoftirqd 内核态线程中调度处理。除此之外，和 BH 相比，softirq 机制充分利用了 SMP 系统的性能和特点，同一个 softirq 的中断处理函数可以并发地在多个 CPU 上同时执行（这也意味着 softirq 的执行函数编写需要考虑相对复杂的同步问题），所以我们也可以认为 softirq 就是一种 fully-SMP 版本的 BH 机制。目前驱动中主要是块设备和网络设备在使用软中断，另外一种更常用的下半部技术 tasklet 实际上是 fully-SMP 的 softirq 的简化版本，其本质依然是 softirq。

软中断最终的执行由 `__do_softirq()` 函数完成。没有引入 `PREEMPT_RT` 情况下主线内核的具体的执行点情况总结如下：

- 在硬中断执行完毕刚刚开中断的时候，以 ARM 架构为例，缺省没有强制线程化（`force_irqthreads` 默认为 0）的执行流程如下：`handle_IRQ()` -> `irq_exit()` -> `__irq_exit_rcu()` -> `invoke_softirq()`` –> `__do_softirq()`。
- 在 ksoftirqd 内核态线程被调度中被执行：`run_ksoftirqd()` -> `__do_softirq()`。
- 在所有内核执行路径中可能会使能软中断处理的地方，具体来说就是调用诸如 `local_bh_enable()` 这些函数中。函数调用序列为`local_bh_enable()`` -> `_local_bh_enable_ip()` -> `do_softirq()`` -> `do_softirq_own_stack` -> `__do_softirq()`。

在 `__do_softirq()` 函数中会根据当前系统 softirq 的 pending 情况决定是直接运行还是推后到 ksoftirqd 内核线程中执行，这里的直接运行应该还是在中断上下文中，只不过是在下半部所以是开中断的。另外一种推迟到 ksoftirqd 内核线程中执行则是在任务上下文中执行。

如果是推迟到 ksoftirqd 内核线程中执行，也就是参与了内核任务调度，这对于实时系统来说是存在 “确定性” 保障的，但是如果是在中断上下文中运行则存在 “不确定性”。这种 “不确定性” 给编写需要执行抢占的实时任务的用户带来了麻烦，同样明显违背了实时系统所依赖的 “确定性” 要求。

## 3.2. 改进方法

`PREEMPT_RT`` 补丁解决这个问题的想法很简单，和处理硬中断类似，只要我们把所有的软中断都放到 ksoftirqd 内核线程中运行，这样就可以按优先级参与内核的统一调度了。但由此可见，这么做会对那些原本通过在中断上下文中执行用来加速处理的块设备、网络设备来说会降低吞吐量上的处理效率，但这对于实时系统来说是无法避免的，因为保证实时性和处理效率本身是一对矛盾的综合体。

具体代码上，和硬中断实时性改造一样，在 5.3 开启 PREEMPT_RT 的主线化工作后，内核 “softirq: Make softirq control and processing RT aware”（commitid：8b1c04acad082dec76f3f8f7e1fa13493d6cbb79）针对 softirq 为 `PREEMPT_RT`` 抢占模式重写了一些关键函数，包括 `invoke_softirq()`、`__local_bh_enable_ip()` 等。以 `invoke_softirq()` 为例，在 `PREEMPT_RT` 下，`invoke_softirq()` 变得很简单，只会唤醒 ksoftirqd 内核线程进行处理，不再有可能在中断上下文中执行 `__do_softirq()` 函数，摘录如下：

```cpp
static inline void invoke_softirq(void)
{
	if (should_wake_ksoftirqd())
		wakeup_softirqd();
}

#else /* CONFIG_PREEMPT_RT */
......
static inline void invoke_softirq(void)
{
	if (ksoftirqd_running(local_softirq_pending()))
		return;

	if (!force_irqthreads() || !__this_cpu_read(ksoftirqd)) {
#ifdef CONFIG_HAVE_IRQ_EXIT_ON_IRQ_STACK
		/*
		 * We can safely execute softirq on the current stack if
		 * it is the irq stack, because it should be near empty
		 * at this stage.
		 */
		__do_softirq();
#else
		/*
		 * Otherwise, irq_exit() is called on the task stack that can
		 * be potentially deep already. So call softirq in its own stack
		 * to prevent from any overrun.
		 */
		do_softirq_own_stack();
#endif
	} else {
		wakeup_softirqd();
	}
}
......
```

FIXME：目前内核中 ksoftirqd 任务的调度策略和优先级我发现并没有针对 `PREEMPT_RT` 做特殊处理，难道和其他内核态任务相同？（普通内核任务 `SCHED_NORMAL`，优先级为 120，具体代码可以参考 `kthread_create_on_node()`）。记得以前的 `PREEMPT_RT` 的补丁会调整了 ksoftirqd 的调度策略为 `SCHED_FIFO`，实时优先级为较低的值 98，这样 ksoftirqd 既可以比普通任务更优先调度但也不会过于抢占高优先级的实时任务。


# 4. 针对锁同步机制的实时化改造

## 4.1. 锁同步机制存在的不确定性问题

- mutex：linux 中传统的互斥锁（mutex）保护的临界区是不可以被抢占的。那么就存在这么一种情况，以 UP 为例，假设一个低优先级的任务先通过获得互斥锁进入临界区，那么由于获取互斥锁会禁止抢占，所以该处理器使用权被其独占，另外一个更高优先级的任务即使没有访问该临界区的需要也不得不等待该低优先级的任务，直到低优先级的任务离开临界区才有可能抢占该处理器的使用。这种情况下对于高优先级任务是不公平的，同时也引入了抢占上的 “不确定性”。

- spinlock：在 SMP 上，自旋锁的禁止抢占行为还会带来一个问题。假设一个双核的处理器环境下，两个任务 A 和 B 共享一个临界区，A 在处理器 CPU1 上先获得锁后进入临界区执行，此时如果 B 在 CPU2 上开始运行，也要获取同一把自旋锁访问该临界区，由于 B 进入自旋等待前也会禁止 CPU2 上的抢占 ，如果 A 在其临界区内执行时间存在不可控性（更严重的是由于 spinlock 并不禁止中断，如果 A 所在的处理器上频繁发生中断也会给 A 在临界区内的执行时间带来延迟），则此时如果另外一个更高优先级的 C 任务也需要运行，则其会因为 CPU1 和 CPU2 都禁止了抢占导致无法及时获得处理器的问题。

- 优先级反转（Priority Inversion）：参考另一篇笔记 [《笔记：优先级反转（Priority Inversion）和 优先级继承（Priority Inheritance）》][2]。

## 4.2. 改进方法

PREEMPT_RT 解决锁同步问题的基本措施是引入 rtmutex。首先简单回顾一下内核中 rtmutex 的发展历史。Mutex 最早在内核 2.6.16 版本由 Ingo Molnar 引入替代 Semaphore，简化了内核互斥下的实现机制。随后为了解决用户态的优先级反转问题在 2.6.18 中内核又吸收了 `PREEMPT_RT` 补丁中提出的支持 PI 算法的 `rt_mutex`。再后来 `PREEMPT_RT` 补丁中为了解决内核态的优先级反转问题，将 mutex 全部替换为 `rt_mutex`。

从 5.15 开始，`PREEMPT_RT` 补丁中有关利用 `rtmutex` 对内核锁相关代码的修改被正式合入内核主线（"locking/rtmutex: Add mutex variant for RT", commitID: bb630f9f7a7d43869e4e7f5e4c002207396aea59），不再作为游离于内核主线之外的 patch 的一部分。

以下描述摘录自 ["KernelNewbies" 针对 5.15 中 “1.7. Real-time locking progress”][5] 的描述。
> In this release, one of the most important pieces, the bulk of the locking code, has been merged. When PREEMPT_RT is enabled, the following locking primitives are substituted by RT-Mutex based variants: mutex, ww_mutex, rw_semaphore, spinlock and rwlock.

简单分析如下：

- `PREEMPT_RT` 对 spinlock 的改造思路是：保留了原来原始的 spinlock 实现，即 `raw_spinlock_t` 结构体，并新建了一个文件 `include/linux/spinlock_types_raw.h` 用于存放 `raw_spinlock_t` 结构体的定义。`raw_spinlock_t` 这个类型定义从 2.6.14 开始就引入了，保留该原始类型还是必要的，因为在内核中依然存在需要调用原始方式自旋锁对临界区进行保护的地方，譬如调度器，rtmutex 的实现，以及一些调试和跟踪的代码。

  使能了 `PREEMPT_RT` 抢占模式后，原来的自旋行为被替换为可睡眠的 rtmutex，而且调用 `spin_lock()` 也不会禁止抢占了。如果还是明确希望自己的临界区采用经典的自旋加禁止抢占行为进行保护，则需要在编程时显式地将自旋锁定义为 `raw_spinlock_t` 类型，并且将 `spin_lock()`/`spin_unlock()` 也显式地替换为 `raw_spin_lock()`/`raw_spin_unlock()`。这么做的原因主要是为了在补丁中减少代码的修改量，因为经过分析几乎 99% 的原使用 spinlock 的地方都可以采用 rtmutex 进行替换，只对剩下的 1% 的还是要保留原自旋锁行为的代码进行修改相对来说还是合算的。

  Spinlock 的实时化改造，可以有效解决前面介绍的 spinlock 在 SMP 上的问题，，首先 CPU1 和 CPU2 都不会被禁止抢占，所以高优先级的任务 C 必定有机会抢占处理器，其次任务 B 此时也不会进入自旋，而是在没有获得互斥量的情况下会进入休眠，这对 CPU2 来说未尝不是一件好事。总而言之，spinlock 的改造对减少高优先级实时任务的抢占延迟会有改进。

- `PREEMPT_RT` 对 spinlock 的改造思路其主要的修改可以关注 `include/linux/mutex.h`。如果没有使能 `PREEMPT_RT`，那么保持原来的 `struct mutex`` 的定义，如果使能了 `PREEMPT_RT`，则用 `rt_mutex_base`` 代替了原来的 spinlock 实现机制。

  ```cpp
  #ifndef CONFIG_PREEMPT_RT
  ......
  struct mutex {
  	......
  	raw_spinlock_t		wait_lock;
	......
  };
  ......
  #else /* !CONFIG_PREEMPT_RT */
  /*
   * Preempt-RT variant based on rtmutexes.
   */
  #include <linux/rtmutex.h>
  
  struct mutex {
  	struct rt_mutex_base    rtmutex;
	......
  };
  ......
  ```

  参考 spinlock 的实时化改造措施，针对 mutex，采用 rtmutex 代替 `raw_spinlock_t` 实现 mutex 后，mutex 在 UP 上会遇到的问题也会得到改善，因为 rtmutex 不会禁止抢占，所以没有和低优先级任务共享临界区的高优先级的任务完全可以抢占低优先级的任务而避免了延迟。

- 优先级反转（Priority Inversion）：这个不必多说，rtmutex 天然支持优先级继承算法解决了 PI。

- FIXME: rw_semaphore，rwlock，seqlock_t 有待补充。

# 5. 参考文档

[1]:./20230805-linux-preemption-models.md
[2]:./20230804-linux-pi-pi.md
[3]:./20230727-rt-linux.md
[4]:https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=70e6e1b971e46f5c1c2d72217ba62401a2edc22b
[5]: https://kernelnewbies.org/Linux_5.15
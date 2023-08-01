![](./diagrams/logo-linux.png)

文章标题：**笔记：Linux 内核的抢占模型**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

本笔记总结了 Linux 内核的抢占模型，包括分析了这些抢占类型中抢占动作的实现机制。注意本文分析的抢占模型中不包括 `PREEMPT_RT`，有关 Real Time 抢占，我打算另外起一篇总结。

代码部分基于 Linux 5.15.36；涉及架构时以 ARM 为例子。

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 内核抢占的定义](#2-内核抢占的定义)
- [3. 内核默认的抢占模式 (Preemption Models)](#3-内核默认的抢占模式-preemption-models)
- [4. `PREEMPT_NONE`](#4-preempt_none)
- [5. `PREEMPT_VOLUNTARY`](#5-preempt_voluntary)
- [6. `PREEMPT`](#6-preempt)
	- [6.1. 临界区的保护](#61-临界区的保护)
	- [6.2. 临界区的界定](#62-临界区的界定)
	- [6.3. 内核抢占发生的调度点](#63-内核抢占发生的调度点)
- [7. 小结](#7-小结)

<!-- /TOC -->

# 1. 参考文档

# 2. 内核抢占的定义

Linux 内核调度器的主要工作就是安排相对多的任务在有限的相对少的处理器上运行，将当前处理器上运行的任务切换出来，将其他就绪的任务切换到处理器上运行，整个过程称之为 “调度”（或者叫 “任务切换”）。

“任务切换” 分为自愿(voluntary)和强制(involuntary)两种。通常自愿切换是指任务由于等待某种资源，将 state 改为非 RUNNING 状态后，调用 `schedule()` 主动让出 CPU；而强制切换则是任务状态仍为 RUNNING 却失去 CPU 使用权，触发条件包括任务时间片用完、有更高优先级的任务就绪等，而第二种强制切换就是本文关心的 “抢占”。

一个任务是否可以被抢占的前提是每个任务的 `thread_info` 的成员 `flags` 上的 `TIF_NEED_RESCHED` 标志位是否被使能。内核调度器通过检查这个标志位判断当前处理器上的任务是否可以被换出。这个标志通过 [set_tsk_need_resched()][15] 进行设置，通过 [clear_tsk_need_resched()][16] 进行清除。可以搜索这两个函数获知 `TIF_NEED_RESCHED` 被设置和清除的代码位置。

# 3. 内核默认的抢占模式 (Preemption Models)

参考 [`kernel/Kconfig.preempt`][1] 文件，主线内核支持三种抢占模式：

- `PREEMPT_NONE`：缺省模式。支持传统的用户态抢占但不支持内核态抢占。可以保证在要求大吞吐量和大计算量下的高处理速度，但无法保证实时系统所要求的确定性，调度延迟较大，主要用于服务器环境。
- `PREEMPT_VOLUNTARY`：通过增加显式抢占点允许低优先级的任务自愿放弃处理器从而提供一定程度的内核态抢占功能。改进了调度延迟，对计算性能稍有损失，比较适合用于有交互性要求的桌面计算环境。
- `PREEMPT`：提供非临界区内核路径下主动抢占低优先级任务的能力。对内核的吞吐量和开销有一定的影响但提高了对交互事件的响应能力。比较适合于多媒体桌面计算环境和对延迟性能要求在毫秒级别的嵌入式系统。
- `PREEMPT_RT`: 实现完全可抢占，支持 Real-Time。该选项是 `PREEMPT_RT`` 补丁的一部分，该补丁（包括该模式选项）从 5.3 内核开始逐渐加入主线，截至本文的 6.4 主线内核仍然没有完全合入。本文暂时不深入分析该抢占模式。

下面分别来看一下主线内核对这前面三种抢占模式的代码实现。

# 4. `PREEMPT_NONE`

顾名思义，该抢占方式从字面意思上理解就是内核不主动发起抢占，除非内核的执行路径自己调用 `schedule()` 函数主动申请让出处理器。当然这里的不抢占（NONE）并不是说非自愿的强制性调度完全不会发生，实际上在该方式下，每次从系统调用返回的前夕以及每次用户态中断返回的前夕内核还是有机会发起抢占的，但是内核态发生的中断不会引起抢占调度。这么做的好处是简单，确保了多任务环境下内核的可重入性问题，但带来的负面作用就是不支持内核态抢占，换句话说该模式下只支持用户态抢占。支持用户态抢占可以避免诸如我们在编写应用程序时进入死循环而导致整个系统无反应的情况，因为只要有中断发生，当中断发生并返回前夕内核会安排一个抢占点，适时地对任务进行调度，譬如时间片到期则抢占当前死循环任务从而避免整个系统僵死。但是如果我们是在内核态进入死循环则无法被抢占。从实时系统角度分析会带来如下问题：

- 一个执行路径调用系统调用函数后进入内核态运行后是无法抢占的，这违反了实时系统 “确定性” 的要求。因为每个内核系统调用函数内部虽然经过大量的优化，但这些函数具体运行所花费的时长毕竟存在并且也不可能考虑具体的某个实时应用的需求，一旦该系统调用函数的执行时长超过了应用可以忍受的上限又不可以被更高优先级别的实时任务抢占就会出现延迟的不可控情况。
- 同样的问题在中断中也会发生，譬如一个低优先级的任务在用户态运行过程中是可以被高优先级的实时任务抢占，但假设该低优先级的任务在用户态运行期间，本地发生中断，由于中断优先级总是最高，抢占了该低优先级的任务并开始执行一段不可预期的中断服务程序，由于中断发生在内核态也是不可抢占的，这也会产生高优先级的实时任务被延迟的情况。

这里主要分析一下 `PREEMPT_NONE` 方式下可以发生抢占的代码控制点：

- 第一个地方是在系统调用返回的前夕：参考 [`arch/arm/kernel/entry-common.S`][2]，关键代码如下：

  ```asm
  ret_fast_syscall:
  __ret_fast_syscall:
  ......
  	ldr	r1, [tsk, #TI_FLAGS]		@ re-check for syscall tracing
  	movs	r1, r1, lsl #16
  	bne	fast_work_pending
  ......
  ENDPROC(ret_fast_syscall)
  
  	/* Ok, we need to do extra processing, enter the slow path. */
  fast_work_pending:
  	str	r0, [sp, #S_R0+S_OFF]!		@ returned r0
  	/* fall through to work_pending */
  ......
  	/* Slower path - fall through to work_pending */
  ......
  slow_work_pending:
  	mov	r0, sp				@ 'regs'
  	mov	r2, why				@ 'syscall'
  	bl	do_work_pending
  	cmp	r0, #0
  	beq	no_work_pending
  	movlt	scno, #(__NR_restart_syscall - __NR_SYSCALL_BASE)
  	ldmia	sp, {r0 - r6}			@ have to reload r0 - r6
  	b	local_restart			@ ... and off we go
  ENDPROC(ret_fast_syscall)
  ......
  ENTRY(vector_swi)
  ......
  	invoke_syscall tbl, scno, r10, __ret_fast_syscall
  ......
  ENDPROC(vector_swi)
  ```

  大致发生逻辑流程为：
  - ARM Linux 系统利用 SWI 指令来发起系统调用从用户空间进入内核空间，入口是在 `vector_swi`，呼叫系统调用的操作被封装在 [`invoke_syscall`][3] 这个 macro 中，仔细读这个 marco，我们会发现它在实际系统调用函数（`@ call sys_* routine`）之前会设置返回地址（`@ return address`）为 `__ret_fast_syscall`。这样当系统调用执行完毕后，处理器会跳转到`__ret_fast_syscall` 处继续执行。

  - 在 `__ret_fast_syscall` 函数中检查当前任务在退出系统调用之前是否有 pending 的工作(`@ re-check for syscall`)，譬如是否有信号未处理，是否有任务调度的需求，都通过检查该任务所对应的 thread_info 结构体的 flags 字段来判断(`TI_FLAGS`)，如果该字段上有对应的位被置 1 则说明有 pending 的事务需要处理，流程跳转到 `fast_work_pending`。 这个函数实际上没有做什么，主要步骤继续往下运行到 `slow_work_pending`

  - 在 `slow_work_pending` 函数中会调用 [`do_work_pending()`][4], 这是一个 c 函数，定义在 `arch/arm/kernel/signal.c` 中。这个函数会具体检查关心的每一位，并分别处理，这里我们关心的是 `_TIF_NEED_RESCHED`，如果需要调度则最终会调用 `schedule()` 函数，具体的调度和上下文切换由该函数去处理。

- 第二个地方是在用户态中断返回的地方：参考 [`arch/arm/kernel/entry-armv.S`][5]，流程如下：

  当在用户态发生中断时，IRQ 异常跳转到 `__irq_usr` 处。

  ```asm
  __irq_usr:
  	usr_entry
  	kuser_cmpxchg_check
  	irq_handler
  	get_thread_info tsk
  	mov	why, #0
  	b	ret_to_user_from_irq
   UNWIND(.fnend		)
  ENDPROC(__irq_usr)
  ```
    
  其中 `irq_handler` 是内核通用中断入口，该函数最终会调用我们 `request_irq()` 注册的中断处理函数 ISR。中断服务处理完后会调用 `get_thread_info` 获取当前任务的 `thread_info` 结构。最后执行 `ret_to_user_from_irq` 函数，所以关键是看这个函数。这个函数定义在 `arch/arm/kernel/entry-common.S`。如下，可见基本逻辑和前面分析的系统调用结束后的处理逻辑类似，如果存在 pending 的工作，譬如 `_TIF_NEED_RESCHED`，则最终会调用 `slow_work_pending`，进而调用 `schedule()` 执行任务切换。

  ```asm
  ENTRY(ret_to_user_from_irq)
  	ldr	r1, [tsk, #TI_FLAGS]
  	movs	r1, r1, lsl #16
  	bne	slow_work_pending
  no_work_pending:
  ......
  ENDPROC(ret_to_user_from_irq)
  ```

- 第三个地方就是内核中其他所有主动调用 `schedule()` 的代码，这里不再赘述。

# 5. `PREEMPT_VOLUNTARY`

从上节的分析我们可以看出对于需要处理高密度大运算量的计算环境，譬如服务器，`PREEMPT_NONE` 是不错的选择。但即使对于类似网络服务器这类应用，内核也应该以合理的速度响应重要的事件。譬如，如果一个网络请求到达，需要守护进程处理，那么该请求不应该被执行繁重磁盘 IO 操作的数据库过度延迟。在不考虑引入内核态抢占的前提下，如何提高系统整体的响应速度有以下两种改进思路：

- 思路一：找出内核态下耗时的路径，尽可能地优化并缩短其执行路径
- 思路二：基本上，内核中耗时长的操作不应该完全占据整个系统。相反，它们应该不时地检测（通过内核调度器）是否有别的更急需处理器的任务需要运行，并在必要的情况下通知调度器主动自愿地（Voluntary）出让处理器。

思路一明显不是长久之计而且是否可以优化到用户满意的情况也无法预期。`PREEMPT_VOLUNTARY` 方式采用的是思路二。而且对于具体的实现方法内核主线最终采纳的是由 Ingo Molnar 提供的补丁。该补丁并没有在内核中添加很多代码来增加新的调度点（（scheduling points），而是改造了 2.6 内核中已经存在的 [`might_sleep`][6] 函数，本质上是改造了 [`might_sleep`][6] 内部会调用的 [`might_resched`][7] 这个宏。

```cpp
#ifdef CONFIG_PREEMPT_VOLUNTARY

extern int __cond_resched(void);
# define might_resched() __cond_resched()

#elif defined(CONFIG_PREEMPT_DYNAMIC)
......
#else

# define might_resched() do { } while (0)

#endif /* CONFIG_PREEMPT_* */
```

暂时不考虑 `CONFIG_PREEMPT_DYNAMIC` 的情况，没有启用 `PREEMPT_VOLUNTARY` 开关时 `might_resched` 什么都不做，一旦打开 `PREEMPT_VOLUNTARY` 开关后，`might_resched` 会被替换为调用 [`_cond_resched()`][8] 函数。该函数内部会检查是否设置了 `TIF_NEED_RESCHED` 标志而有条件地调用调度函数 `__schedule()`。

内核中出于调试的目的在很多长路径中已经安插了对 `might_sleep()` 函数的调用，Ingo Molnar 对该函数内部进行改造后，一旦 `PREEMPT_VOLUNTARY` 开关打开，所有调用 `might_sleep()` 的地方就自然而然地成为了内核新增的调度点了。内核已经仔细检查过，对那些长时间运行的函数在适当的地方都插入了对 `might_sleep()` 函数的调用。即使没有打开显式的内核抢占开关（`CONFIG_PREEMPT`），采用该机制后也可以保证较高的响应速度。

顺便提一下 `/proc/<PID>/status` 中记录了每个进程的强制切换和自愿切换的次数。譬如：

```bash
$ grep voluntary_ctxt_switches  /proc/2429997/status
voluntary_ctxt_switches:	208
nonvoluntary_ctxt_switches:	0
```

如果一个进程的自愿切换占多数，意味着它对 CPU 资源的需求不高，或者说它不是一个计算密集型的任务；反之如果一个进程的强制切换占多数，表明它对 CPU 的依赖较强。

# 6. `PREEMPT`

从对 `PREEMPT_NONE` 和 `PREEMPT_VOLUNTARY` 的分析可以看出，这两种方式下内核都是不可以抢占的。所谓的内核不可以抢占意味这在内核态下运行的代码会一直占有处理器运行直到完成，此间其他高优先级的任务无法获得调度。虽然内核的代码效率被仔细检查过了，但内核的不可抢占性仍然是一个巨大的潜在隐患会造成无法预期的抢占延迟问题（延迟往往会达到几百 ms 以上）。当面临 ms 级别精度的实时应用需求时就力不从心了。

`PREEMPT` 模式的引入其设计目标就是尽量确保一个高优先级的实时任务在变得可以运行（Runnable）而需要获得处理器执行其代码时总是能够被 **及时** 分配到处理器资源。在用户态下的抢占已经存在的前提下，所剩的就是要实现在内核态的抢占了。

虽然我们说目标是要实现内核代码 100% 可以被抢占，但事实也不是真的 100%，总有那么一些地方是需要严格保护而不可以被抢占的，所以 `PREEMPT` 模式的设计思路就是要找出内核中那些不可以被抢占的地方，我们称之为 "临界区（Critical Section）"，并把它们标识并保护起来，那么剩下的地方就都是可以被抢占的了。

## 6.1. 临界区的保护

在 `PREEMPT` 方式下提供了如下机制对临界区进行保护：

- 抢占发生的前提是要确保此次抢占是安全的。所谓安全即确保当前（current）任务没有持有锁，否则在 current 任务没有释放锁的前提下被切换出来可能会发生死锁。所以首先修改对应每个任务的 `thread_info` 结构体（体系架构相关），增加一个抢占计数器（[`preempt_count`][9]）来帮助内核跟踪该任务的状态来确认当前是否可以被抢占，获取锁前会去增加抢占计数器的值，抢占发生前会去检查 `preempt_count` 是否为 0, 值 为 0 时说明本任务可以被抢占，大于 0 时不可以被抢占。

  ```cpp
  struct thread_info {
  	......
  	int			preempt_count;	/* 0 => preemptable, <0 => bug */
  	......
  };
  ```

  在此基础上内核提供了封装函数对抢占计数器进行操作。这些 API 包括 `preempt_count_inc`/`preempt_count_dec`，这些函数工作很简单，就是对 `thread_info` 结构体的 `preempt_count` 成员加一或者减一。

- 提供给最终用户使用的接口 API 来控制禁止抢占和恢复抢占，这套 API 包括其变种有好几个，具体参考内核头文件`include/linux/preempt.h`。其中两个最典型的 API 函数 `preempt_disable()`和 `preempt_enable()` 为例，代码如下： 
  
  ```cpp
  #ifdef CONFIG_PREEMPT_COUNT
  
  #define preempt_disable() \
  do { \
  	preempt_count_inc(); \
  	barrier(); \
  } while (0)
  ......
  #ifdef CONFIG_PREEMPTION
  #define preempt_enable() \
  do { \
  	barrier(); \
  	if (unlikely(preempt_count_dec_and_test())) \
  		__preempt_schedule(); \
  } while (0)
  ...
  #else /* !CONFIG_PREEMPTION */
  #define preempt_enable() \
  do { \
  	barrier(); \
  	preempt_count_dec(); \
  } while (0)
  ...
  #endif /* CONFIG_PREEMPTION */
  ...
  #else /* !CONFIG_PREEMPT_COUNT */
  
  /*
   * Even if we don't have any preemption, we need preempt disable/enable
   * to be barriers, so that we don't have things like get_user/put_user
   * that can cause faults and scheduling migrate into our preempt-protected
   * region.
   */
  #define preempt_disable()			barrier()
  ......
  #define preempt_enable()			barrier()
  ......
  #endif /* CONFIG_PREEMPT_COUNT */
  ```

  `CONFIG_PREEMPT_COUNT` 这个配置开关和 `CONFIG_PREEMPTION`` 是捆绑的，当我们选择了 `PREEMPT`` 或者 `PREEMPT_RT`` 时会自动 `select PREEMPTION`，而 `PREEMPTION`` 会进一步自动 `select PREEMPT_COUNT`。这意味着 `PREEMPT`` 模式下 `CONFIG_PREEMPT_COUNT`` 也会被打开，所以以上代码可以认为在没有打开 PREEMPT 开关时 `preempt_xxxx()`` 这些函数都是空函数，一旦启用 `PREEMPT`` 模式后就会影响内核逻辑。

  `PREEMPT`` 模式开启情况下 `preempt_disable()` 函数的逻辑非常简单，核心逻辑就是调用 `preempt_count_inc()` 增加抢占计数器的值，`preempt_enable()` 函数的逻辑稍微复杂一点，除了执行反操作调用 `preempt_count_dec_and_test()` 外还会调用`__preempt_schedule()`，该函数有可能会执行实际的任务调度完成抢占。 

- 以上机制完备后，对于临界区的保护可以采用以下简单的方式：

  ```cpp
  preempt_disable();
  /* Critical Section */
  preempt_enable();
  ```

## 6.2. 临界区的界定

有了保护临界区的方法后，剩下的就是找出内核中所有的临界区然后再用上述方法标识出来即可。内核中的临界区是有限并可以界定的，具体包括如下情形：
- (1) 硬中断处理程序执行过程包括软中断下半部（不考虑中断线程化的情况）。
- (2) 调度器内部执行过程。
- (3) Fork 一个新的进程的过程。
- (4) 其他不可以被多个任务同时访问（重入）的内核临界区。

(1)~(3) 这些情况在内核中的代码位置都比较好确定，一旦确定直接用 `preempt_disable()`/`preempt_enable()` 保护起来就可以了。唯独对于 (4) 的情况所涉及的临界区在内核现存代码中其数目很多，散布在内核中各个角落。如果一处一处去甄别修改显然是不现实的。所幸的是我们知道在使能了内核抢占后，在 UP（Uni-Processor）环境下内核临界区所存在的重入问题，和 SMP（Symmetric MultiProcessing）环境下的临界区重入问题，其本质是一样的。内核早期已经建立起来的一套针对 SMP 条件下对临界区进行保护的机制框架完全可以被复用。换句话说，在解决 SMP 的临界区保护问题中，内核引入了自旋锁（spinlock）机制，所有需要保护的临界区已经被形如以下的代码标记出来了。

```cpp
DEFINE_SPINLOCK(mr_lock);
......
spin_lock(&mr_lock);
/* Critical Section */
spin_unlock(&mr_lock);
```

这些已经通过 spinlock 标识出来的临界区既是 SMP 需要保护的对象，也是内核抢占需要保护的对象。基于以上思想，为了解决 (4)，我们只需要对 spinlock 的 API 进行内部改造即可复用 SMP 的成果。同样 spinlock 的 API 包括各种变种有好几个，我们主要分析[`spin_lock()`][10] 和 [`spin_unlock()`][11]。

spinlock 的内核实现比较复杂，因为要区分 SMP 版本和 UP 版本。针对 SMP 版本，`spin_lock()/spin_unlock()` 最终调用的是 `__raw_spin_lock()`/`__raw_spin_unlock()`，这两个函数定义在 [`include/linux/spinlock_api_smp.h`][12]，如下所示。抛开和自旋锁本身实现相关的代码我们不做分析，最关键的是我们看到在 `spin_lock()` 和 `spin_unlock()` 函数中分别内嵌了对 `preempt_disable()` 和 `preempt_enable()` 函数的调用。

```cpp
static inline void __raw_spin_lock(raw_spinlock_t *lock)
{
	preempt_disable();
	spin_acquire(&lock->dep_map, 0, 0, _RET_IP_);
	LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
}

static inline void __raw_spin_unlock(raw_spinlock_t *lock)
{
	spin_release(&lock->dep_map, _RET_IP_);
	do_raw_spin_unlock(lock);
	preempt_enable();
}
```

针对 UP 版本，`spin_lock()/spin_unlock()` 最终调用的是 `__LOCK()/__UNLOCK()`，定义在 [`include/linux/spinlock_api_up.h`][13]，摘录如下。可见在自旋锁的 UP 版本上除了保留了对抢占的操作外，实际上的自旋都被优化掉了，原因很简单，自旋锁设计的初衷就是一旦一个处理器上有执行路径获取了该锁，那么其他所有处理器，包括该处理器本身也不能拥有该锁。对于 UP 的情形，我们又禁止了抢占，天生已经满足了以上要求，多余的自旋操作自然就不需要了。 

```cpp
#define ___LOCK(lock) \
  do { __acquire(lock); (void)(lock); } while (0)

#define __LOCK(lock) \
  do { preempt_disable(); ___LOCK(lock); } while (0)
......
#define ___UNLOCK(lock) \
  do { __release(lock); (void)(lock); } while (0)

#define __UNLOCK(lock) \
  do { preempt_enable(); ___UNLOCK(lock); } while (0)
```

综上所述，内核的所有临界区的情况都可以通过 `preempt_disable()/preempt_enable()` 这对函数直接或者间接地保护起来。

## 6.3. 内核抢占发生的调度点

如果在内核中执行路径被阻塞了（譬如发起和设备的读调用），或者显式地调用了 `schedule()` 函数（类似 `PREEMPT_VOLUNTARY` 做的那样），内核抢占也会发生。这些形式的内核抢占一直存在，不是我们在 `PREEMPT 模式下实现的内核抢占所考虑的重点。对于 `PREEMPT` 方式下的内核抢占，我们关注的重点总结下来会发生在以下两点上：

- 一处是参考 `preempt_enable()` 代码，可以看出，当内核路径离开临界区的时候，内核是有机会去检查当前任务是否是可以被抢占的，如果条件满足，最终会调用 `__preempt_schedule()` 完成抢占调度。
- 另外一处是在内核态中断退出的点上，这里的内核路径和前面提到的用户态中断的处理是不同的。参考 [`arch/arm/kernel/entry-armv.S`][14] 中如下代码。

  在内核态发生 IRQ 中断后，会跳转到 `__irq_svc` 函数入口，在该函数中会同样获取该中断所借用的任务所对用的 `thread_info` 结构体，并判断其中的 flag 和抢占计数器值，如果满足抢占条件则调用 `svc_preempt`，在该函数中会调用 `preempt_schedule_irq()` 执行实际的抢占调度动作。这些代码逻辑都包在 `CONFIG_PREEMPTION` 宏里所以只有在使能了内核抢占后才会执行。

```asm
__irq_svc:
	svc_entry
	irq_handler

#ifdef CONFIG_PREEMPTION
	ldr	r8, [tsk, #TI_PREEMPT]		@ get preempt count
	ldr	r0, [tsk, #TI_FLAGS]		@ get flags
	teq	r8, #0				@ if preempt count != 0
	movne	r0, #0				@ force flags to 0
	tst	r0, #_TIF_NEED_RESCHED
	blne	svc_preempt
#endif

	svc_exit r5, irq = 1			@ return from exception
 UNWIND(.fnend		)
ENDPROC(__irq_svc)

	.ltorg

#ifdef CONFIG_PREEMPTION
svc_preempt:
	mov	r8, lr
1:	bl	preempt_schedule_irq		@ irq en/disable is done inside
	ldr	r0, [tsk, #TI_FLAGS]		@ get new tasks TI_FLAGS
	tst	r0, #_TIF_NEED_RESCHED
	reteq	r8				@ go again
	b	1b
#endif
```

# 7. 小结

历史上 Linux 内核的设计一直是支持抢占的，最初的实现就是目前默认的 `PREEMPT_NONE`，只不过那时候没有其他的抢占模式，所以不需要专门定义这个配置选项。在这种模式下抢占点发生的场景包括：
- 从系统调用返回的前夕；
- 从用户态中断返回的前夕；
- 自己调用 `schedule()` 主动让出处理器。

2.5.4 时引入 `PREEMPT` 模式，但配置散落在各个 arch 的 Kconfig 中，譬如 `arch/arm/Kconfig`、`arch/i386/Kconfig`。当然完整的 `PREEMPT` 模式的实现经历了一个相对较长的阶段，最终达到的效果就是在 `PREEMPT_NONE` 的基础上实现了在内核态也能够抢占，包括：
- 退出内核临界区时；
- 从内核态中断返回的前夕。

从 2.6.13 开始，引入 `PREEMPT_VOLUNTARY`, 并且整理出 `kernel/Kconfig.preempt` 这个新文件，用于定义所有的抢占相关的配置项。改造内核中已经存在的 `might_sleep` 函数，内部调用 `might_resched` 增加调度点。


[1]:https://elixir.bootlin.com/linux/v5.15.36/source/kernel/Kconfig.preempt
[2]:https://elixir.bootlin.com/linux/v5.15.36/source/arch/arm/kernel/entry-common.S
[3]:https://elixir.bootlin.com/linux/v5.15.36/source/arch/arm/kernel/entry-header.S#L379
[4]:https://elixir.bootlin.com/linux/v5.15.36/source/arch/arm/kernel/signal.c#L601
[5]:https://elixir.bootlin.com/linux/v5.15.36/source/arch/arm/kernel/entry-armv.S
[6]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/kernel.h#L131
[7]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/kernel.h#L94
[8]:https://elixir.bootlin.com/linux/v5.15.36/source/kernel/sched/core.c#L8162
[9]:https://elixir.bootlin.com/linux/v5.15.36/source/arch/arm/include/asm/thread_info.h#L54
[10]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/spinlock.h#L361
[11]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/spinlock.h#L401
[12]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/spinlock_api_smp.h
[13]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/spinlock_api_up.h
[14]:https://elixir.bootlin.com/linux/v5.15.36/source/arch/arm/kernel/entry-armv.S
[15]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/sched.h#L2001
[16]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/sched.h#L2006
![](./diagrams/logo-linux.png)

文章标题：**笔记：Linux 任务优先级（priority）总结**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

本笔记主要是总结了 Linux 上提供的用于设置任务调度的 API。

另一篇笔记 [《笔记：Linux 任务调度相关 API 总结》][1] 中总结了用户态系统调用接口中和任务优先级相关的 API。本文继续结合内核实现总结一下任务优先级的相关概念。

Linux 代码部分基于版本 5.15.36。

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 内核中任务优先级的设置](#2-内核中任务优先级的设置)

<!-- /TOC -->

# 1. 参考文档 


# 2. 内核中任务优先级的设置

内核中任务优先级相关的数据结构定义在著名的结构体 [`task_struct`][2] 中，摘录如下:

```cpp
struct task_struct {
	...
	int				prio;
	int				static_prio;
	int				normal_prio;
	unsigned int			rt_priority;
	...
};
```

`static_prio` 和 `rt_priority` 分别代表普通("normal")任务和实时("real-time")任务的 "静态优先级"，这里所谓 "静态优先级" 代表存放用户设置的优先级值，系统运行过程中除非用户改变否则不会自己变化。参考 [《笔记：Linux 任务调度相关 API 总结》][1]，我们通过 `nice()` 或者`sched_setscheduler()`(policy 取值为 `SCHED_OTHER`/`SCHED_BATCH`/`SCHED_IDLE`)设置的就是存放在 `static_prio` 的值，用 `sched_setscheduler()`(policy 取值为 `SCHED_FIFO`/`SCHED_RR`) 设置的就是存放在 `rt_priority` 的值。由于 “normal” 和 “real-time” 他们两面向上层用户的接口的表达方式不同，“normal” 任务的 `static_prio`，取值范围是 [-20, 19]，值越小优先级越高，“real-time” 任务的 `rt_priority` 取值范围是[0, 99]，值越大优先级越高（需要注意的是用户态接口并不接受 0，实际用户态接口可以接受的范围是 [1，99]）。

以上两个变量是对应存放用户态的设置值，但进入内核态后计算时有必要统一一下，这就是引入 `normal_prio` 的原因。统一成值越小优先级越高并且范围是 [0,139]，因此，`normal_prio` 也可以理解为: 统一了单位的 "静态优先级"。

- 对于 “normal” 任务的优先级转换：`static_prio`` 的取值范围（即我们常说的用户态的 nice 值）是：[-20, 19]，从 `static_prio` 值转换为内核的 `normal_prio`：`normal_prio = 120 + nice`。

- 对于 “real-time” 任务的优先级转换：`rt_priority` 取值范围是 [0, 99]，从`rt_priority` 值转换为内核的 `normal_prio`：`normal_prio = 99 - rt_priority`

综上所述：从内核的角度，任务优先级，即 `normal_prio` 一共 140 个档。其中 real-time 任务的优先级范围是 [0, 99]，“normal” 任务的优先级范围是 [100, 139]。统一为：值越小意味着优先级别越高，优先级越高，则任务优先被内核调度运行。

`prio` 是动态优先级，或者叫 “有效优先级”(effective priority)，顾名思义，在内核中实际判断任务优先级时用的是该参数，调度器考虑的优先级也是它。该值会被初始化为和 `normal_prio` 的值相同。但在实际调度过程中调度器可能会修改进程的 `prio` 的值。譬如在解决优先级反转（priority inversion）问题中，当使用实时互斥量时，内核会临时提高任务的 `prio` 的值，释放互斥量后恢复成原先初始化的值。而 `normal_prio` 在系统运行过程中其值是不变的。


[1]:./20230802-linux-sched-api.md
[2]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/sched.h#L773
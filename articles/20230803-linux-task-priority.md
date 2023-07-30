![](./diagrams/logo-linux.png)

文章标题：**笔记：Linux 任务优先级（priority）总结**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

另一篇笔记 [《笔记：Linux 任务调度相关 API 总结》][1] 中总结了用户态系统调用接口中和任务优先级相关的 API。本文继续结合内核实现总结一下任务优先级的相关概念，并总结了命令行查看任务优先级的方法。

Linux 代码部分基于版本 5.15.36。

<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 内核中任务优先级的设置](#2-内核中任务优先级的设置)
- [3. 命令行查看任务的 policy 和 priority](#3-命令行查看任务的-policy-和-priority)

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

- 对于 “normal” 任务的优先级转换：`static_prio` 的取值范围（即我们常说的用户态的 nice 值）是：[-20, 19]，从 `static_prio` 值转换为内核的 `normal_prio` 的公式是：`normal_prio = 120 + nice`。

- 对于 “real-time” 任务的优先级转换：`rt_priority` 取值范围是 [0, 99]，从`rt_priority` 值转换为内核的 `normal_prio` 的公式是：`normal_prio = 99 - rt_priority`

综上所述：从内核的角度，任务优先级，即 `normal_prio` 一共 140 个档 [0, 139]。其中 real-time 任务的优先级范围是 [0, 99]，“normal” 任务的优先级范围是 [100, 139]。统一为：值越小意味着优先级别越高，优先级越高，则任务优先被内核调度运行。

`prio` 是动态优先级，或者叫 “有效优先级”(effective priority)，顾名思义，在内核中实际判断任务优先级时用的是该参数，调度器考虑的优先级也是它。该值会被初始化为和 `normal_prio` 的值相同。但在实际调度过程中调度器可能会修改进程的 `prio` 的值。譬如在解决优先级反转（priority inversion）问题中，当使用实时互斥量时，内核会临时提高任务的 `prio` 的值，释放互斥量后恢复成原先初始化的值。而 `normal_prio` 在系统运行过程中其值是不变的。


# 3. 命令行查看任务的 policy 和 priority

最常用的是 ps 命令, 但我发现使用 ps 查看任务的 priority 比较复杂，因为历史原因，其显示的 PRI 那一列的值常常需要自己转化才能和内核级别的那些 priority 对应上。

我建议采用如下方式查看，假设任务 PID 为 1960700。

如果要看 policy：

```bash
$ cat /proc/1960700/sched | grep ^policy
policy                                       :                    0
```

这里的值的对应关系，参考 [内核代码对 policy 的定义][3]，摘录如下:

```c
/*
 * Scheduling policies
 */
#define SCHED_NORMAL		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_BATCH		3
/* SCHED_ISO: reserved but not implemented yet */
#define SCHED_IDLE		5
#define SCHED_DEADLINE		6
```

如果要看 priority：
```bash
$ cat /proc/1960700/sched | grep ^prio
prio                                         :                  120
```

这里得到的值 prio 就是 `task_struct.prio`。具体参考 [内核代码][4]。

看 policy 也可以用 ps:

```bash
$ ps -cL
    PID     LWP CLS PRI TTY          TIME CMD
......
1960700 1960700 TS   19 pts/0    00:00:00 ping
......
```

- `-c`: 显示 scheduling policy，这里取值如下, 记不得可以 `man ps`` 并搜索关键字 CLS：

  >  - not reported
  >  - TS  SCHED_OTHER
  >  - FF  SCHED_FIFO
  >  - RR  SCHED_RR
  >  - B   SCHED_BATCH
  >  - ISO SCHED_ISO
  >  - IDL SCHED_IDLE
  >  - DLN SCHED_DEADLINE
  >  - ?   unknown value

- `-L`: 加这个选项目的是因为 task 本质上是线程，所以 task priority 本质上是 thread 上的属性。

- `-PRI`: 这里我们会发现，ps 显示的 PRI 的值并不是我们前面所述的内核级别的 `normal_prio` 或者 `prio`，如果不想自己换算，可以先不管这个域。


[1]:./20230802-linux-sched-api.md
[2]:https://elixir.bootlin.com/linux/v5.15.36/source/include/linux/sched.h#L773
[3]:https://elixir.bootlin.com/linux/v5.15.36/source/include/uapi/linux/sched.h#L111
[4]:https://elixir.bootlin.com/linux/v5.15.36/source/kernel/sched/debug.c#L1030
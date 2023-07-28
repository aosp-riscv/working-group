![](./diagrams/logo-linux.png)

文章标题：**笔记：Linux 任务调度相关 API 总结**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

本笔记主要是总结了 Linux 上提供的用于设置任务调度的 API。
<!-- TOC -->

- [1. 参考文档](#1-参考文档)
- [2. 设置调度策略（policy）和优先级（priority）](#2-设置调度策略policy和优先级priority)
	- [2.1. nice](#21-nice)
	- [2.2. setpriority/getpriority](#22-setprioritygetpriority)
	- [2.3. sched_setscheduler/sched_getscheduler](#23-sched_setschedulersched_getscheduler)
	- [2.4. sched_setattr/sched_getattr](#24-sched_setattrsched_getattr)
	- [2.5. sched_get_priority_max/sched_get_priority_min](#25-sched_get_priority_maxsched_get_priority_min)
- [3. 其他（未总结）](#3-其他未总结)

<!-- /TOC -->

# 1. 参考文档 


# 2. 设置调度策略（policy）和优先级（priority）

先简单总结一下：

| API | 是否 POSIX |
|-----|-----------|
|nice | 是 |
|setpriority/getpriority | 是 |
|sched_setscheduler/sched_getscheduler | 是 （注：SCHED_BATCH and SCHED_IDLE 不是 POSIX 定义的，是 Linux 自己定义的）|
|sched_setattr/sched_getattr | 否 |

## 2.1. nice

```bash
NAME
       nice - change process priority

SYNOPSIS
       #include <unistd.h>

       int nice(int inc);
```

nice 这个名字有点意思，inc 取值范围 [-20, +19]，这个值越低导致任务的优先级越高，值越高任务优先级越低。可以这么理解，给的 inc 值越高越 nice，说明最终的任务 “脾气越好”，因为此时任务优先级越低，越容易被其他高优先级的任务抢占，让出 cpu。

一般情况下，如果要设置一个负数 inc，即提高任务的优先级需要特权权限。

命令行的命令有 nice 和 renice，但是 API 只有 nice。命令行的 nice 和 renice 内部都是通过调用 `nice()` 实现的。

## 2.2. setpriority/getpriority

```bash
man 2 setpriority

NAME
       getpriority, setpriority - get/set program scheduling priority

SYNOPSIS
       #include <sys/time.h>
       #include <sys/resource.h>

       int getpriority(int which, id_t who);
       int setpriority(int which, id_t who, int prio);
```

是 `nice()` 的等价实现，但是扩展了 `nice()` 的功能。

## 2.3. sched_setscheduler/sched_getscheduler

```bash
NAME
       sched_setscheduler, sched_getscheduler - set and get scheduling policy/parameters

SYNOPSIS
       #include <sched.h>

       int sched_setscheduler(pid_t pid, int policy,
                              const struct sched_param *param);

       int sched_getscheduler(pid_t pid);
```

进一步增强，可以设置 policy，当前支持的 policy 包括：
- "normal" (i.e., non-real-time) scheduling policies
  - `SCHED_OTHER`
  - `SCHED_BATCH`
  - `SCHED_IDLE`
- "real-time" policies
  - `SCHED_FIFO`
  - `SCHED_RR`

设置 priority 时：
- "normal" (i.e., non-real-time) scheduling policies：只能设置 priority 为 0
- "real-time" policies：
  > This is a number in  the  range  returned  by
  > calling sched_get_priority_min(2) and sched_get_priority_max(2) with the 
  > specified policy.  On Linux, these system calls return, respectively, 1 
  > and 99.
  这个值越小优先级越高。

## 2.4. sched_setattr/sched_getattr

```bash
man 2 sched_setattr

NAME
       sched_setattr, sched_getattr - set and get scheduling policy and attributes

SYNOPSIS
       #include <sched.h>

       int sched_setattr(pid_t pid, struct sched_attr *attr,
                         unsigned int flags);

       int sched_getattr(pid_t pid, struct sched_attr *attr,
                         unsigned int size, unsigned int flags);
```

是对 sched_setscheduler/sched_getscheduler 的进一步增强：

当前支持的 policy 包括：
- "normal" (i.e., non-real-time) scheduling policies
  - `SCHED_OTHER`
  - `SCHED_BATCH`
  - `SCHED_IDLE`
- "real-time" policies
  - `SCHED_FIFO`
  - `SCHED_RR`
- deadline scheduling policy（Linux 特供）
  - `SCHED_DEADLINE`

设置 priority 时：
- "normal" scheduling policies：可以通过 `sched_attr.sched_nice` 针对 `SCHED_OTHER` 或者 `SCHED_BATCH` 设置优先级，范围是 -20 (high priority) to +19 (low priority); SCHED_IDLE 不需要设置 priority。
- "real-time" policies：可以通过 `sched_attr.sched_priority` 针对 `SCHED_FIFO` 或者 `SCHED_RR` 设置优先级，范围根据 sched_get_priority_min(2) and sched_get_priority_max(2) 获取;
- deadline scheduling policy：`SCHED_DEADLINE` 不需要设置 priority，但有其他参数需要额外设置，譬如 `sched_runtime/sched_deadline/sched_period`。

## 2.5. sched_get_priority_max/sched_get_priority_min

辅助 API，参考 `sched_setscheduler/sched_getscheduler` 中的描述。

# 3. 其他（未总结）

- `sched_rr_get_interval`

- `sched_yield`

- `sched_setaffinity/sched_getaffinity`

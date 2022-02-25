![](./diagrams/android-riscv.png)

文章标题：**AOSP-RISCV 的开源仓库在 Gitee 上新建了镜像**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲
<!-- TOC -->


<!-- /TOC -->

前阵子在知乎上给大家介绍了我们在移植 AOSP 到 RISC-V 上的第一步: 
[第一个 RISC-V 上的“Android 最小系统”](./20201120-first-rv-android-mini-system.md)。
​
目前所有的工作成果都是开源在 Github 上的，移植改动涉及的子仓库达到 9 个，所有源码下载下
来达到 537M+，由于 Github 的访问速度较慢，这么多东西要全部下载下来会非常的慢。

为了方便国内的小伙伴访问我们的开源仓库，最近我们在国内知名的开源托管仓库平台 Gitee 上也
建立了相应的镜像：<https://gitee.com/aosp-riscv>

注意：由于 2021 年 1 月 平头哥的 [开源工作](https://github.com/T-head-Semi/aosp-riscv)，
我们于 2021 年初暂停了 PLCT lab 的相关 AOSP 移植工作。所有原 <https://github.com/aosp-riscv> 
和 <https://gitee.com/aosp-riscv> 下的代码仓库（除了 working-group）都备份到 Gitee 
的 aosp-riscv-bionic-porting 组织下，具体网址是：<https://gitee.com/aosp-riscv-bionic-porting>。

大家如果对我们 2021 年 1 月份之前的工作（具体参考 
[《第一个 RISC-V 上的“Android 最小系统”》](./20201120-first-rv-android-mini-system.md) 
这篇文章的介绍）感兴趣的话。相关的代码可以参考上面提到的 aosp-riscv-bionic-porting。

原有的 <https://github.com/aosp-riscv> 和 <https://gitee.com/aosp-riscv> 将用于我
们新阶段的 AOSP for RISC-V 的开发工作，

另外，所有 AOSP for RISC-V 的开源工作状态我们都会定期更新在 github/gitee 上，大家可以
在线访问，以 github 的网页为例:

中文版：<https://github.com/aosp-riscv/working-group/blob/master/README_zh.md>
​

或者

英文版：<https://github.com/aosp-riscv/working-group/blob/master/README.md>
​

欢迎对 AOSP 以及 RISC-V 移植感兴趣的小伙伴来围观、参与，给我们提 PR/issue。


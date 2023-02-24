**笔记：如何搭建 Chrome 开发环境**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [下载 Chrome 代码。](#下载-chrome-代码)
- [Android NDK](#android-ndk)
- [开发 Clang for Chrome](#开发-clang-for-chrome)

<!-- /TOC -->

# 下载 Chrome 代码。

目前我们的开发主仓库是 `https://github.com/aosp-riscv/chromium`, 开发的集成分支是 `riscv64_109.0.5414.87_dev`。

假设我们的工作目录是 `$WS`
```
mkdir $WS/chromium && cd $WS/chromium
fetch --nohooks android
cd src
git remote add aosp-riscv git@github.com:aosp-riscv/chromium.git
git fetch aosp-riscv
git checkout riscv64_109.0.5414.87_dev
build/install-build-deps.sh --android
gclient runhooks
```

这样就取到我们的开发代码了，你可以在此基础上继续创建你自己的开发分支。


# Android NDK

构建 Chrome 的过程中依赖于 Android NDK，因为目前 Google 还没有发布支持 riscv64 的 NDK，所以我们采用一个自己临时制作的 NDK。具体方法如下：

```
cd $WS/chromium/src/third_party
git clone git@github.com:aosp-riscv/android-ndk-ci.git
git clone git@github.com:aosp-riscv/riscv64-linux-android.git
```

`android-ndk-ci` 这个仓库下载下来后还要修改一个符号链接，因为我们对于 riscv64 的 sysroot 实际使用的是另一个仓库 `riscv64-linux-android`，这个仓库 fork 自 <https://android.googlesource.com/toolchain/prebuilts/sysroot/platform/riscv64-linux-android>，目前在其基础上补充了一些 libc++/libc++abi 的库，这些库 Google 暂未提供，但是构建 Clang 的 runtime 库时需要。

```
cd android-ndk-ci/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/
rm riscv64-linux-android
ln -s $WS/chromium/src/third_party/riscv64-linux-android/usr/lib/riscv64-linux-android/ riscv64-linux-android 
```

最后我们将 Chrome 原先自带的 `android_ndk` 目录替换为我们自己的 ndk
```
cd $WS/chromium/src/third_party
mv android_ndk android_ndk.bak
ln -s ./android-ndk-ci ./android_ndk
```


# 开发 Clang for Chrome

如果要开发 Clang for Chrome。我们目前的集成分支是 `riscv64_109.0.5414.87_dev_clang`。
构建 Clang for Chrome 时需要注意还要下载一些 fuchia 相关的依赖，具体参考 [《笔记：Clang for Chromium 构建分析》][1] 的 "3. 构建 Clang for Chromium"。

[1]:../../articles/20230201-chrome-clang-build.md

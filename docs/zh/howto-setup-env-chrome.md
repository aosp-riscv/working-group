**笔记：如何搭建 Chrome 开发环境**

- 作者：汪辰
- 联系方式：<unicorn_wang@outlook.com> / <wangchen20@iscas.ac.cn>

文章大纲

<!-- TOC -->

- [下载 Chrome 代码。](#下载-chrome-代码)
	- [Install depot_tools](#install-depot_tools)
	- [Get the code](#get-the-code)
	- [Apply the patches](#apply-the-patches)
	- [Switch Android NDK](#switch-android-ndk)
- [构建 Chrome](#构建-chrome)
- [构建 WebView](#构建-webview)
- [开发 Clang for Chrome](#开发-clang-for-chrome)

<!-- /TOC -->

# 下载 Chrome 代码。

目前我们的开发主仓库是 <https://github.com/aosp-riscv/chromium>, 开发的集成分支是 `riscv64_109.0.5414.87_dev`。

可以采用如下步骤搭建 chromium 的开发环境，假设我们的工作目录是 `$WS`。

## Install depot_tools

“Install depot_tools” 的步骤省略，具体参考 [Checking out and building Chromium for Android][2]。

## Get the code

注意，以下步骤特别是涉及的子仓库修改可能随着开发的进展不断补充和完善。

```shell
mkdir $WS/chromium && cd $WS/chromium
fetch --nohooks android
cd src
git checkout 109.0.5414.87
build/install-build-deps.sh --android
gclient sync
```

注意运行 `install-build-deps.sh` 需要 sudo，其内部实际上就是在执行 `apt install`。

最后一步 `gclient sync` 除了会根据我们 checkout 出来的版本进一步同步一些依赖仓库外，还会在最后执行 hooks（类似于单独执行 `gclient runhooks`）。

## Apply the patches

我们目前开发的代码基线是 `109.0.5414.87`。以上步骤完成后我们还需要在以上版本基础上打上我们的修改补丁。具体步骤如下：

**注意**：目前我们采用切换到 aosp-riscv 仓库的开发分支的方式给对应仓库打补丁。采用该方式需要注意的一个问题是，如果在执行以下操作后再运行 `gclient sync` 则相关 checkout 的分支信息会丢失，需要重新手动再 checkout 一次。

FIXME: 以后可以考虑和 <https://github.com/starfive-tech/riscv-nwjs-patch> 一样采用 patch 方式提供。

```shell
cd $WS/chromium/src
git remote add aosp-riscv git@github.com:aosp-riscv/chromium.git
git fetch aosp-riscv
git checkout riscv64_109.0.5414.87_dev

cd $WS/chromium/src/third_party/ffmpeg/
git remote add aosp-riscv git@github.com:aosp-riscv/ffmpeg.git
git fetch aosp-riscv 
git checkout riscv64_109.0.5414.87_dev 

cd $WS/chromium/src/v8
git remote add aosp-riscv git@github.com:aosp-riscv/v8.git
git fetch aosp-riscv
git checkout riscv64_109.0.5414.87_dev 

cd $WS/chromium/src/third_party/angle/
git remote add aosp-riscv git@github.com:aosp-riscv/angle.git
git fetch aosp-riscv 
git checkout riscv64_109.0.5414.87_dev 
```

## Switch Android NDK

此外，构建 Chrome 的过程中依赖于 Android NDK，因为目前 Google 还没有发布支持 riscv64 的 NDK，所以除了打以上补丁，我们还需要采用一个自己临时制作的 NDK 来替换它。具体方法如下：

```shell
cd $WS/wangchen/dev-chrome/chromium/src/third_party
git clone git@github.com:aosp-riscv/android-ndk-ci.git
mv android_ndk android_ndk.chrome
ln -s ./android-ndk-ci/ ./android_ndk
```

**注意**：进一步操作之前还需要修改 ndk 中两个符号链接。

- $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin
- $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/lib

需要将以上两个符号链接指向实际的我们构建用的 llvm/clang 工具链的下的 bin/lib 目录。目前在 android-ndk-ci 仓库中我们木有存放 llvm/clang 工具链的内容，因为有些文件的 size 太大，超出了 github 的容量限制。所以 llvm/clang 工具链我们是单独提供的。当然也可以从源码开始自己制作一份（具体见本文的 “开发 Clang for Chrome” 章节介绍）。

假设你使用的工具链(二进制可执行程序 `bin/clang` 所在的目录)是 `$MY_CLANG`。执行以下命令：

```shell
rm $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin
ln -s $MY_CLANG/bin $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin
rm $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/lib
ln -s $MY_CLANG/lib $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/lib
```

这样就取到我们的开发代码了，你可以在此基础上继续创建你自己的开发分支。

# 构建 Chrome

为了方便开发，在我们 aosp-riscv 的 chromium 仓库的 `riscv64_109.0.5414.87_dev` 分支上提供了一个简单的 `Makefile`。可以直接使用：

假设你使用的工具链(二进制可执行程序 `bin/clang` 所在的目录)是 `$MY_CLANG`。

然后就可以执行如下命令
```shell
cd $WS/chromium/src
make distclean
make gn CLANG=$MY_CLANG
make ninja
```

执行完成后在 `$WS/chromium/src/out/riscv64/apks/` 下会生成 `ChromePublic.apk`。

# 构建 WebView

和构建 Chrome 类似，假设你使用的工具链(二进制可执行程序 `bin/clang` 所在的目录)是 `$MY_CLANG`。

然后就可以执行如下命令
```shell
cd $WS/chromium/src
make distclean
make gn CLANG=$MY_CLANG
make ninja T=trichrome_webview_apk
```

因为我们目前针对 riscv64 的移植目标系统 API level 最小是 29，所以这里我们构建目标设置为 `trichrome_webview_apk`，具体参考 [WebView Build Instructions][3]。

执行完成后在 `$WS/chromium/src/out/riscv64/apks/` 下会生成 `TrichromeWebView64.apk` 和 `TrichromeLibrary64.apk`。

# 开发 Clang for Chrome

如果要开发 Clang for Chrome。我们目前的集成分支是 `riscv64_109.0.5414.87_dev_clang`。
构建 Clang for Chrome 时需要注意还要下载一些 fuchia 相关的依赖，具体参考 [《笔记：Clang for Chromium 构建分析》][1] 的 "3. 构建 Clang for Chromium"。

[1]:../../articles/20230201-chrome-clang-build.md
[2]:https://chromium.googlesource.com/chromium/src.git/+/HEAD/docs/android_build_instructions.md
[3]:https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/build-instructions.md

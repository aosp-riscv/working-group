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
		- [备注：另一个专门为 `build_ffmpeg.py` 制作的 NDK](#备注另一个专门为-build_ffmpegpy-制作的-ndk)
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
build/install-build-deps-android.sh
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
这样就取到我们的开发代码了，你可以在此基础上继续创建你自己的开发分支。

## Switch Android NDK

此外，构建 Chrome 的过程中依赖于 Android NDK，因为目前 Google 还没有发布支持 riscv64 的 NDK，所以除了打以上补丁，我们还需要采用一个自己临时制作的 NDK 来替换它。

NDK 的仓库位置在: <https://github.com/aosp-riscv/toolchain-prebuilts-ndk-r23>: 基于 <https://github.com/riscv-android-src/toolchain-prebuilts-ndk-r23> 修改后的版本，如果希望在 aosp12 for riscv64 的开发板上运行可以选用这个 NDK。

切换方法如下：
```shell
cd $WS/chromium/src/third_party
git clone git@github.com:aosp-riscv/toolchain-prebuilts-ndk-r23.git
mv android_ndk android_ndk.chrome
ln -s ./toolchain-prebuilts-ndk-r23/ ./android_ndk
```

### 备注：另一个专门为 `build_ffmpeg.py` 制作的 NDK

在移植 chromium 过程中，我们还提供了一个 ndk：<https://github.com/aosp-riscv/android-ndk-ci>: 基于 Google upstream 的 <https://ci.android.com/builds/branches/aosp-master-ndk/grid> (linux) 版本制作的 ndk。目前该 NDK 只用于产生 ffmpeg 的 auto generated files。具体背景参考以下两个 commitments 的 comment：
- <https://github.com/aosp-riscv/ffmpeg/commit/de7399e94ab2c148e6fd3a4f68b6cbc6354af697>
- <https://github.com/aosp-riscv/ffmpeg/commit/15e5ff93d073fa7c9e11b1a7b1a46a786e591ad7>

在运行 `build_ffmpeg.py` 脚本文件时记得切换 NDK 如下，同时 **注意在构建 chromium 时还要恢复原来的 NDK**：

```shell
cd $WS/chromium/src/third_party
git clone git@github.com:aosp-riscv/android-ndk-ci.git
mv android_ndk android_ndk.chrome
ln -s ./android-ndk-ci/ ./android_ndk
```
**注意**：进一步操作之前还需要修改该 NDK 中两个符号链接。

- `$WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin`
- `$WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/lib`

需要将以上两个符号链接指向实际的我们构建用的 llvm/clang 工具链的下的 bin/lib 目录。目前在 android-ndk-ci 仓库中我们没有存放 llvm/clang 工具链的内容，因为有些文件的 size 太大，超出了 github 的容量限制。所以 llvm/clang 工具链我们是单独提供的。当然也可以从源码开始自己制作一份（具体见本文的 “开发 Clang for Chrome” 章节介绍）。

假设你使用的工具链(二进制可执行程序 `bin/clang` 所在的目录)是 `$MY_CLANG`。执行以下命令：

```shell
rm $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin
ln -s $MY_CLANG/bin $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin
rm $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/lib
ln -s $MY_CLANG/lib $WS/chromium/src/third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/lib
```

**注意**：目前为了方便，已经将 auto generated files 也一并提交到 <https://github.com/aosp-riscv/ffmpeg> 仓库中去了，所以本小节的描述仅供开发者在修改了 ffpeg 仓库的 `build_ffmpeg.py` 文件并且需要重新生成 auto generated files 时参考使用。

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

没有指定 T，默认 target 为 `chrome_public_apk`，执行完成后在 `$WS/chromium/src/out/riscv64/apks/` 下会生成 `ChromePublic.apk`。

我们也可以构建 Content Shell APK For Android，可以执行如下命令：
```shell
cd $WS/chromium/src
make distclean
make gn CLANG=$MY_CLANG
make ninja T=content_shell_apk
```
当然也可以在构建 `chrome_public_apk` 的基础上再编译 `content_shell_apk`，不用从头重新编译。执行完成后在 `$WS/chromium/src/out/riscv64/apks/` 下会生成 `ContentShell.apk`。


# 构建 WebView

和构建 Chrome 类似，假设你使用的工具链(二进制可执行程序 `bin/clang` 所在的目录)是 `$MY_CLANG`。

如果我们目前针对 riscv64 的移植目标系统 API level 是 29+，则构建目标设置为 `trichrome_webview_apk`，执行命令如下：

```shell
cd $WS/chromium/src
make distclean
make gn CLANG=$MY_CLANG
make ninja T=trichrome_webview_apk
```

执行完成后在 `$WS/chromium/src/out/riscv64/apks/` 下会生成 `TrichromeWebView64.apk` 和 `TrichromeLibrary64.apk`。

如果我们针对 riscv64 的移植目标系统 API level 是 (24-28)，则可以将构建目标设置为 `monochrome_public_apk`。具体参考 [WebView Build Instructions][3]。执行完成后在 `$WS/chromium/src/out/riscv64/apks/` 下会生成 `MonochromePublic64.apk`。

目前暂时不支持 `system_webview_apk`。

# 开发 Clang for Chrome

构建 Clang for Chrome 的开发环境具体参考 [《笔记：Clang for Chromium 构建分析》][1] 的 "3. 构建 Clang for Chromium"。注意这个文档只是描述了如何基于上游的代码构建。如果要构建针对 riscv64 的 android 上的 chrome 所用的 clang，同样需要将 chromium 的仓库切换到集成分支 `riscv64_109.0.5414.87_dev`。同时，如果只是构建 clang，则不需要对其他的仓库（譬如 ffmpeg、v8 等 Apply patches），但需要 Switch Android NDK，因为构建 clang 时，特别是 compiler_rt 时会依赖于 Android NDK。

[1]:../../articles/20230201-chrome-clang-build.md
[2]:https://chromium.googlesource.com/chromium/src.git/+/HEAD/docs/android_build_instructions.md
[3]:https://chromium.googlesource.com/chromium/src/+/HEAD/android_webview/docs/build-instructions.md

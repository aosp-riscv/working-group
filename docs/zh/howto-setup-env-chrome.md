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

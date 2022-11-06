**How to build Chromium RISCV64**

<!-- TOC -->

- [1. Hardware environment](#1-hardware-environment)
- [2. Install depot\_tools](#2-install-depot_tools)
- [3. Get the code](#3-get-the-code)
- [4. Install additional build dependencies](#4-install-additional-build-dependencies)
- [5. Checkout the branch](#5-checkout-the-branch)
- [6. Run the hooks](#6-run-the-hooks)
- [7. Build Rust toolchain](#7-build-rust-toolchain)
- [8. Setting up the build](#8-setting-up-the-build)
- [9. Build Chromium](#9-build-chromium)

<!-- /TOC -->

# 1. Hardware environment

All operations in this article have been verified under the following system environment:

```
$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 20.04.5 LTS
Release:        20.04
Codename:       focal
```

# 2. Install depot\_tools

Clone the `depot_tools` repository:

```shell
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

Add `depot_tools` to the end of your PATH (you will probably want to put this
in your `~/.bashrc` or `~/.zshrc`). Assuming you cloned `depot_tools`
to `/path/to/depot_tools`:

```shell
export PATH="$PATH:/path/to/depot_tools"
```

# 3. Get the code

Create a `toolchain` directory for the special Android RISCV64 toolchain to substitute
Chromium official toolchain.
```shell
mkdir ~/toolchain && cd ~/toolchain
git clone --branch riscv-ndk-release-r23 https://github.com/riscv-android-src/platform-prebuilts-clang-host-linux-x86.git
git clone --branch riscv-llvm-r416183_dev https://github.com/riscv-android-src/toolchain-prebuilts-ndk-r23.git
```

Create a `chromium` directory for the checkout and change to it (you can call
this whatever you like and put it wherever you like, as long as the full path has no spaces):

```shell
mkdir ~/chromium && cd ~/chromium
```

`fetch` the code

```
gclient root
gclient config --spec 'solutions = [
  {
    "name": "src",
    "url": "https://github.com/aosp-riscv/chromium.git",
    "managed": False,
    "custom_deps": {},
    "custom_vars": {"checkout_nacl": False},
  },
]
target_os = ["android"]
'
gclient sync --nohooks
```

When `fetch` completes, it will have created a hidden `.gclient` file and a
directory called `src` in the working directory. The remaining instructions
assume you have switched to the `src` directory:

```shell
cd src
git submodule foreach 'git config -f $toplevel/.git/config submodule.$name.ignore all'
git config --add remote.origin.fetch '+refs/tags/*:refs/tags/*'
git config diff.ignoreSubmodules all
```

# 4. Install additional build dependencies

Once you have checked out the code, run

```shell
build/install-build-deps-android.sh
```

to get all of the dependencies you need to build on Linux, *plus* all of the
Android-specific dependencies (you need some of the regular Linux dependencies
because an Android build includes a bunch of the Linux tools and utilities).

# 5. Checkout the branch

Checkout riscv64-android-12.0.0\_dev branch where Chromium RISCV64 support is involved.

```shell
git checkout riscv64-android-12.0.0_dev
```

# 6. Run the hooks

Once you've run `install-build-deps` at least once, you can now run the
Chromium-specific hooks, which will download additional binaries and other
things you might need:

```shell
gclient runhooks
```

# 7. Prepare the toolchain

```shell
rm -rf third_party/llvm-build/Release+Asserts/*
cp -rf ~/toolchain/platform-prebuilts-clang-host-linux-x86/clang-r416183c2/bin third_party/llvm-build/Release+Asserts/
cp -rf ~/toolchain/platform-prebuilts-clang-host-linux-x86/clang-r416183c2/lib64 third_party/llvm-build/Release+Asserts/
rm -rf third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64
cp -rf ~/toolchain/platform-prebuilts-clang-host-linux-x86/clang-r416183c2 third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64
ln -s third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/lld third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/ld
cp -rf ~/toolchain/toolchain-prebuilts-ndk-r23/toolchains/llvm/prebuilt/linux-x86_64/sysroot third_party/android_ndk/toolchains/llvm/prebuilt/linux-x86_64/
```

# 8. Setting up the build

To create a build directory which builds Chromium for Android, run:

```shell
mkdir -p out/riscv64
echo 'target_os = "android"
target_cpu = "riscv64"
' > out/riscv64/args.gn
gn args out/riscv64
```

# 9. Build Chromium

Build Chromium with Ninja using the command:

```shell
autoninja -C out/Default chrome_public_apk
```

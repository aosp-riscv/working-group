简单总结一下 hack clang15 for chrome 109 的修改：

基本环境搭建参考：
- [《如何搭建 Chrome 开发环境》](./howto-setup-env-chrome.md)
- 搭建 clang 构建环境，参考 [《笔记：Clang for Chromium 构建分析》](../../articles/20230201-chrome-clang-build.md)

注意我在构建 chrome 和构建 clang 时使用的 ndk 都是 "版本 A"。

具体的 hack 操作包含以下三部分：

- 【1】clang 15 based on `105.0.5195.148`，所做的修改在 https://github.com/unicornx/chromium/tree/dev-chrome-clang-15
  
  ```shell
  b2ad1278f2c0a (HEAD -> dev-chrome-clang-15, unicornx/dev-chrome-clang-15) ignore test first to pass build/package
  f089e8e0df100 disable relaxation for lld
  3d2d828b1c460 test to build clang 15 for riscv
  ```

  注意 3d2d828b1c460 中部分改动如果你是第一次做还需要手动做些修改。可以按照如下步骤：

  我这里是先基于 **原始 105.0.5195.148 的版本** 修改 `tools/clang/scripts/build.py`，
  ```python
      if not args.skip_checkout:
        CheckoutLLVM(checkout_revision, LLVM_DIR)
  +     return
  ```
  加上 return，然后运行 `package.py`,利用脚本下载了 chrome 105.0.5195.148 对应的 llvm 的源码仓库。等 `CheckoutLLVM()` clone 完 llvm 仓库就退出来。
  
  这个克隆下来的 llvm 要打一些patch才能编译。所以此时做 【2】
  
  【2】完成后继续修改 `build.py` 强制以后跳过 checkout 的步骤。
  ```python
  +   args.skip_checkout = True
      if not args.skip_checkout:
        CheckoutLLVM(checkout_revision, LLVM_DIR)
        return
  ```
  所以目前仓库中 `3d2d828b1c460` 反映的是以上操作后最终的修改结果。


- 【2】llvm 仓库打补丁

  clang 15 对应的代码需要打上一些 patch 才能工作。
  
  具体见 https://github.com/unicornx/llvm-project/commits/dev-clang-15-chrome-105
  
  ```shell
  cf580ae7186b (HEAD -> dev-clang-15-chrome-105, unicornx/dev-clang-15-chrome-105) FIXME: just make test passed
  ea54677922fc temp: patched with D145474
  9e447684f528 temp workaround to pass build runtime for riscv
  c4cdb0bc3b5a patch to enable riscv64 for android
  ```

- 【3】chrome 109 的仓库也要做一些修改
  
  基于【1】和【2】可以做出 clang 15，但在编译用这个 clang15 构建 chrome 109 时还要对 chrome 109 做些小修改。
  
  基于当时最新的 aosp-riscv/riscv64_109.0.5414.87_dev，修改分支在 https://github.com/unicornx/chromium/tree/test-clang-15
  
  ```shell
  f090ceb55594f (HEAD -> test-clang-15, unicornx/test-clang-15) now can build apk with clang 15
  ```


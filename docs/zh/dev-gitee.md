**如何基于 Gitee 参与开发**

假设你需要修改的仓库地址是: <https://gitee.com/aosp-riscv/platform_bionic>

大致的步骤如下:

- 首先从这个仓库上 fork 一个你自己的开发仓库，具体操作参考文档 [**“如何 fork 仓库”**](https://gitee.com/help/articles/4128#2%E5%A6%82%E4%BD%95-fork-%E4%BB%93%E5%BA%93)。

- fork 完成后，就可以将自己的仓库下载到本地进行修改开发了。建议在本地仓库里为 remote 添加 upstream，指向上游仓库的地址，为以后更新同步做准备。参考操作步骤如下，假设自己的账号是 robot：

  ```
  $ git clone git@gitee.com:robot/platform_bionic.git
  $ cd platform_bionic/
  $ git remote -v
  origin  git@gitee.com:robot/platform_bionic.git (fetch)
  origin  git@gitee.com:robot/platform_bionic.git (push)
  $ git remote add upstream git@gitee.com:aosp-riscv/platform_bionic.git
  $ git remote -v
  origin  git@gitee.com:robot/platform_bionic.git (fetch)
  origin  git@gitee.com:robot/platform_bionic.git (push)
  upstream        git@gitee.com:aosp-riscv/platform_bionic.git (fetch)
  upstream        git@gitee.com:aosp-riscv/platform_bionic.git (push)
  $ git fetch --all
  ```

- 然后就为自己的仓库创建开发分支，譬如 `develop` ，注意基于最新的集成分支 `riscv64-android-12.0.0_dev` 创建，并在自己的代码没有被合入 `riscv64-android-12.0.0_dev` 之前时刻保持对开发分支的更新和同步。 

- 我们采用标准的 **Gitee “Fork + PullRequest”** 方式进行开发，完成修改后请提交 PullRequest 并通知仓库管理员审核和合并。提交 PullRequest 时注意设置好 **源分支**（这里为 `develop`） 和 **目标分支**（这里为 `riscv64-android-12.0.0_dev`）。具体 PullRequest 的操作参考文档 [**“Fork + PullRequest 模式”**](https://gitee.com/help/articles/4128#article-header0)。

- 合并完成后注意在本地仓库中及时地将本地和 `origin` 的 `riscv64-android-12.0.0_dev` 分支和 `upstream` 进行同步，包括自己的开发分支，确保自己的工作基于最新的版本。下面给一个更新 `riscv64-android-12.0.0_dev` 分支的步骤例子（基于上面为 remote 添加 upstream 的操作）：

  ```
  $ git fetch --all
  $ git checkout riscv64-android-12.0.0_dev
  $ git rebase --onto upstream/riscv64-android-12.0.0_dev --root
  $ git push origin riscv64-android-12.0.0_dev
  ```
  
  更新完成后就可以进入下一个开发周期了。
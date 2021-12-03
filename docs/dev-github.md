**How to participate in development based on Github**

Suppose the address of the warehouse you need to modify is: <https://github.com/aosp-riscv/platform_bionic>

The general steps are as follows:

- First fork a development repository of your own from this repository, refer to online document [**"Fork a repo"**](https://docs.github.com/en/get-started/quickstart/fork-a-repo)。

- After the fork is completed, you can download your own repository to the local for modification and development. It is recommended to add `upstream` to remote in the local repository, pointing to the address of the upstream repository, in preparation for the update and synchronization in the future. The reference operation steps are as follows, assuming that your account is `robot`:

  ```
  $ git clone git@github.com:robot/platform_bionic.git
  $ cd platform_bionic/
  $ git remote -v
  origin  git@github.com:robot/platform_bionic.git (fetch)
  origin  git@github.com:robot/platform_bionic.git (push)
  $ git remote add upstream git@github.com:aosp-riscv/platform_bionic.git
  $ git remote -v
  origin  git@github.com:robot/platform_bionic.git (fetch)
  origin  git@github.com:robot/platform_bionic.git (push)
  upstream        git@github.com:aosp-riscv/platform_bionic.git (fetch)
  upstream        git@github.com:aosp-riscv/platform_bionic.git (push)
  $ git fetch --all
  ```

- Then create a development branch for your own repository, such as `develop`, pay attention to create based on the latest integration branch `riscv64-android-12.0.0_dev`. Keep the development branch updated and synchronized with the integration branch before your changes are merged into `riscv64-android-12.0.0_dev`.

- We follow-up the standard **Github "Fork + PullRequest"** procedure for development. After completing the modification, please submit a PullRequest and notify the repository adminitrator for review and merge. When submitting the PullRequest, pay attention to setting up the **compare branch** (here: `develop`) and the **base branch** (here: `riscv64-android-12.0.0_dev`). For specific PullRequest operations, please refer to online document [**"Collaborating with pull requests"**](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests).


- After the merge is completed, pay attention to timely syncronize the local and `origin`'s `riscv64-android-12.0.0_dev` branches in the local repository with upstream, as well as your own development branch, to ensure that they are based on the latest version. Here is an example of the steps to update the `riscv64-android-12.0.0_dev` branch (based on the above operation of adding upstream to remote):

  ```
  $ git fetch --all
  $ git checkout riscv64-android-12.0.0_dev
  $ git rebase --onto upstream/riscv64-android-12.0.0_dev --root
  $ git push origin riscv64-android-12.0.0_dev
  ```
  
  After the update is complete, you can enter the next development cycle.
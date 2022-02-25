**如何搭建 soong 的基于 vscode 的远程开发调试环境**

注：提供此文档为方便国内参与小伙伴。

<!-- TOC -->

- [1. vscode 的远程开发调试环境](#1-vscode-的远程开发调试环境)
- [2. 安装依赖软件](#2-安装依赖软件)
- [3. 运行 debug](#3-运行-debug)
- [4. 参考文档](#4-参考文档)

<!-- /TOC -->

# 1. vscode 的远程开发调试环境

Server 端建议为 Ubuntu PC ，环境如下：（本文所有操作在以下系统环境下验证通过）

```
$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 18.04.6 LTS
Release:        18.04
Codename:       bionic
```
Client 端运行 vscode IDE 可以自选 PC 系统环境， Windows 、 Linux 皆可。


# 2. 安装依赖软件

Server 端：  
```
$ sudo apt install openssh-server
$ wget https://golang.google.cn/dl/go1.18rc1.linux-amd64.tar.gz && sudo rm -rf /usr/local/go && sudo tar -C /usr/local -xzf go1.18rc1.linux-amd64.tar.gz
$ echo "export PATH=$PATH:/usr/local/go/bin" >> ~/.bashrc
```
至此， Server 端配置完成。下面开始配置 Client 端。  
注：此处使用当前（文档撰写时）最新版本 go1.18rc1 的考量是 riscv 生态正在高速发展，对新引入的特性只在最新版本的软件上才会被支持。另，riscv64 的 llvm-toolchain 的构建已经在使用 go1.18beta1。  

Client 端：  
1)安装 vscode 1.64.2 或更高版本；  
2)启动 vscode 并安装 vscode extension Remote-SSH 在 Client 端本地，安装成功后左侧边栏新增 Remote Explorer ；  
3)将 Server 端作为 Host 添加到 Client 端运行的 vscode 中；  
```
"Remote Explorer" -->
    "SSH TARGETS"-->
        "Add New" -->
            "Enter SSH Connection Command" <ssh YOURUSERNAME@YOURSERVERIP> -->
                "Select SSH configuration file to update" <YOURPATH/.ssh/config> -->
                    "Host added!"
```
4)安装 vscode-server 到 Server 端  
```
"Remote Explorer" -->
    "SSH TARGETS" -->
        "Connect to Host in Current Window" -->
            "Select the platform of the remote host YOURSERVERIP" <Linux> -->
                "Are you sure want to continue" <Continue> -->
                    "Enter password for YOURUSERNAME@YOURSERVERIP" -->
```
至此这会将 vscode-server 程序自动安装到 Server 端的 ~/.vscode-server 目录下，并且 Client 端已通过 vscode extension Remote-SSH 完成登录到 Server 端；  
5)在通过 vscode extension Remote-SSH 已登录到 Server 端的状态下，安装 vscode extension Go 且版本 >0.31，实际是将它安装到了 Server 端；  
6)由于国内用户访问外网受限，可通过设置代理安装 go debug tool go-delve ；打开 vscode Terminal ，然后手动执行如下：  
```
$ go env -w GO111MODULE=on
$ go env -w GOPROXY=https://goproxy.cn,direct
$ echo "export GO111MODULE=on" >> ~/.profile
$ echo "export GOPROXY=https://goproxy.cn" >> ~/.profile
$ source ~/.profile
$ sudo apt install gcc
$ go install github.com/go-delve/delve/cmd/dlv@v1.8.1
```
至此， Client 端配置完成。  


# 3. 运行 debug

如下操作都在 Client 端进行：  
运行 vscode 通过其插件 Remote-SSH 远程访问 Server 端的 AOSP 代码目录  
打开 vscode Terminal 并先执行如下：  
```
$ source build/envsetup.sh
$ SOONG_UI_DELVE=5006 m nothing    //此处5006为自定义值，注意后面 port 值需跟其保持一致
```
然后，  
```
"Run and Debug" -->
    "create a launch.json file" -->
        "Select environment" <Go> -->
            "Choose debug configuration" <Go: Connect to server> -->
                "Enter hostname" <YOURSERVERIP> -->
                    "Enter port" <5006> -->
```
此时 launch.json 被打开并手动加入`"debugAdapter": "dlv-dap", `, 如下：  
```
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Connect to server",
            "type": "go",
            "debugAdapter": "dlv-dap",
            "request": "attach",
            "mode": "remote",
            "remotePath": "${workspaceFolder}",
            "port": 5006,
            "host": "YOURSERVERIP"
        }
    ]
}
```
至此再"F5"，即可开始远程调试 soong 。  

注意：  
"SOONG_UI_DELVE=5006 m nothing" 后，如遇错误如下：  
```
user@Ubuntu1804:~/riscv/aosp/plct/riscv64-android-12.0.0$ SOONG_UI_DELVE=5006 m nothing
API server listening at: [::]:5006
2022-02-23T22:40:06+08:00 warning layer=rpc Listening for remote connections (connections are not authenticated nor encrypted)
2022-02-23T22:40:07+08:00 error layer=debugger can't find build-id note on binary
Go version 1.15.6 is too old for this version of Delve (minimum supported version 1.16, suppress this error with --check-go-version=false)
#### failed to build some targets (1 seconds) ####
```
其原因是基于当前的 riscv64-android-12.0.0 代码， prebuilts/go/linux-x86 目录下的 go 版本太过老旧不满足 go-delve 的版本要求，故临时对 prebuilts/go/linux-x86 建软链接到系统路径下的高版本 go ，此问题解决。具体操作如下：  
```
$ cd ${AOSP}
$ mv prebuilts/go/linux-x86 prebuilts/go/bak.linux-x86
$ ln -s /usr/local/go prebuilts/go/linux-x86    # 此处 /usr/local/go 为前面步骤所安装的高版本 go 的路径

```
**但请记得在编译 AOSP 时回退这部分修改，使用官方提供的 go 版本，此方法仅用于调试 soong**。具体回退操作如下：  
```
$ cd ${AOSP}
$ rm prebuilts/go/linux-x86
$ mv prebuilts/go/bak.linux-x86 prebuilts/go/linux-x86

```
此问题根本解决可等待下一个版本 AOSP 13 中官方提供升级的 go 版本。  


# 4. 参考文档

* https://android.googlesource.com/platform/build/soong/+/master/README.md
* https://github.com/golang/vscode-go/blob/master/docs/debugging.md
* https://code.visualstudio.com/docs/remote/ssh
* https://code.visualstudio.com/api/advanced-topics/remote-extensions
* https://code.visualstudio.com/docs/editor/debugging
* https://goproxy.cn

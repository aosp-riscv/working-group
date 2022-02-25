**How to setup remote debug environment with vscode for soong**

<!-- TOC -->

- [1. Remote debug environment for vscode](#1-Remote-debug-environment-for-vscode)
- [2. Install dependent software](#2-Install-dependent-software)
- [3. Run debug](#3-Run-debug)
- [4. Reference](#4-Reference)

<!-- /TOC -->

# 1. Remote debug environment for vscode

The recommendation of server end is Ubuntu PC as below: (All operations in this article have been verified under the following system environment:)

```
$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 18.04.6 LTS
Release:        18.04
Codename:       bionic
```
The client end runs vscode IDE and can choose either Windows or Linux.


# 2. Install dependent software

Server end:  
```
$ sudo apt install openssh-server
$ wget https://golang.google.cn/dl/go1.18rc1.linux-amd64.tar.gz && sudo rm -rf /usr/local/go && sudo tar -C /usr/local -xzf go1.18rc1.linux-amd64.tar.gz
$ echo "export PATH=$PATH:/usr/local/go/bin" >> ~/.bashrc
```
The configure operation on the server end is finished. Then, it's time to configure the client end.  
Note: Choosing the latest version go1.18rc1 (at the time of writing the documentation) is due to RISCV ecosystem is developing at a high speed and the new features would only be supported on the latest version of softwares. Such as, the riscv64's llvm-toolchain build is already using go1.18beta1.  

Client end:  
1)Install vscode 1.64.2 or newer version;  
2)Run vscode and install vscode extension Remote-SSH on the client end locally; Once the installation succeeds, there is new added "Remote Explorer" on the left side of vscode IDE;  
3)Add the server end as a "Host" to the vscode which is running on the client end;  
```
"Remote Explorer" -->
    "SSH TARGETS"-->
        "Add New" -->
            "Enter SSH Connection Command" <ssh YOURUSERNAME@YOURSERVERIP> -->
                "Select SSH configuration file to update" <YOURPATH/.ssh/config> -->
                    "Host added!"
```
4)Install vscode-server to the server end;  
```
"Remote Explorer" -->
    "SSH TARGETS" -->
        "Connect to Host in Current Window" -->
            "Select the platform of the remote host YOURSERVERIP" <Linux> -->
                "Are you sure want to continue" <Continue> -->
                    "Enter password for YOURUSERNAME@YOURSERVERIP" -->
```
The vscode-server is installed to the path (~/.vscode-server) of the server end automatically. And the client end's vscode logins the server end via vscode extension Remote-SSH;  
5)Install vscode extension Go version > 0.31 when the client end's vscode logins the server end via vscode extension Remote-SSH; The vscode extension Go is installed to the server end actually;  
6)Install go debug tool "go-delve"; Open vscode "Terminal" and execute the below commands:  
```
$ sudo apt install gcc
$ go install github.com/go-delve/delve/cmd/dlv@v1.8.1
```
The configure operation on the client end is finished.  


# 3. Run debug

All below operations are run on the client end:  
Run vscode to login the AOSP folder in the server end via vscode extension Remote-SSH;  
Open vscode "Terminal" and execute the below commands:  
```
$ source build/envsetup.sh
$ SOONG_UI_DELVE=5006 m nothing    // The 5006 is a self-defined value, it should be kept same with the latter port setting value.
```
Then,  
```
"Run and Debug" -->
    "create a launch.json file" -->
        "Select environment" <Go> -->
            "Choose debug configuration" <Go: Connect to server> -->
                "Enter hostname" <YOURSERVERIP> -->
                    "Enter port" <5006> -->
```
The launch.json is opened and add `"debugAdapter": "dlv-dap", ` into the lauch.json mannually, as below：  
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
Now press "F5"，remotely debugging soong with vscode is started.  

Note:  
When "SOONG_UI_DELVE=5006 m nothing", if there is an error as below:  
```
user@Ubuntu1804:~/riscv/aosp/plct/riscv64-android-12.0.0$ SOONG_UI_DELVE=5006 m nothing
API server listening at: [::]:5006
2022-02-23T22:40:06+08:00 warning layer=rpc Listening for remote connections (connections are not authenticated nor encrypted)
2022-02-23T22:40:07+08:00 error layer=debugger can't find build-id note on binary
Go version 1.15.6 is too old for this version of Delve (minimum supported version 1.16, suppress this error with --check-go-version=false)
#### failed to build some targets (1 seconds) ####
```
It's due to the go version of prebuilts/go/linux-x86 provided by riscv64-android-12.0.0 is old.  
And the go version can not match the requirement from go-delve.  
So a temporary solution is to create a symbol link to use the higher version go which is in the system folder /usr/local/go.  
Please apply the solution as below:  
```
$ cd ${AOSP}
$ mv prebuilts/go/linux-x86 prebuilts/go/bak.linux-x86
$ ln -s /usr/local/go prebuilts/go/linux-x86    # The "/usr/local/go" is the path of the installed higher version go in the previous step.

```
**But please revert the above modification before compiling AOSP to use the official provided go in prebuilts/go/linux-x86.**  
The reversion is as below:  
```
$ cd ${AOSP}
$ rm prebuilts/go/linux-x86
$ mv prebuilts/go/bak.linux-x86 prebuilts/go/linux-x86

```
The final solution for the issue is the official providing the newer version go in the coming AOSP 13.


# 4. Reference

* https://android.googlesource.com/platform/build/soong/+/master/README.md
* https://github.com/golang/vscode-go/blob/master/docs/debugging.md
* https://code.visualstudio.com/docs/remote/ssh
* https://code.visualstudio.com/api/advanced-topics/remote-extensions
* https://code.visualstudio.com/docs/editor/debugging

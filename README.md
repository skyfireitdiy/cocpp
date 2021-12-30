# README

## 简介

cocpp是一个基于现代C++的协程库（c++20），其主要特点是易于使用，会用std::thread，就会用cocpp，封装性好，学习成本低。

目前的环境要求为：

- x86_64体系架构
- linux操作系统
- gcc支持C++20

### hello world

一个最简单的Demo：

```C++
#include "cocpp/interface/co.h"

#include <cstdio>
#include <string>

int main()
{
    cocpp::co c1([]() {
        return std::string("Hello ");
    });

    auto ret = c1.wait<std::string>();
    printf("%s", (ret + "World!\n").c_str());
}
```


## 特性介绍

目前cocpp支持的特性说明如下：

- 自由的协程入口函数

	任何样式的函数都可以作为函数签名，无论是有返回值的、无返回值的、无参数的、一个参数的、多个参数，都可以作为协程的入口函数。
- 协程优先级

	协程创建时可以指定协程的优先级，也可以在协程运行过程中修改优先级（优先级0~99，其中0最高，99最低，默认创建的协程优先级为99）。不同优先级间以高优先级协程优先调度，相同优先级的协程轮询调度。
- 多线程负载均衡

	协程可以在不同的线程间迁移实现以负载均衡。
- 死循环强制调度

	当协程环境超过设置的阈值没有调度时，会从外部强制调度。
- 系统调用超时，重新分配协程

	当一个协程由于系统调用而被阻塞时，管理器会将当前线程上其余所有可迁移的协程迁移到其他合适的执行环境中。
- 协程偷取

	当当前线程空闲了，会尝试从其他执行环境偷取协程来执行。
- 协程同步工具

	具有丰富的协程同步工具。信号量、二值信号量、条件变量、互斥锁、递归锁、读写锁等。
- 用于通信的channel

	和Go一样，具有用于协程通信的channel实现。
- 事件

	协程的各个阶段都可以注入处理函数，便于跟踪协程执行情况。
- 共享栈

	多个协程可以共用一个栈空间，以此减少内存使用（共享栈协程不支持迁移）。
- 运行环境绑定

	协程可以与某个运行环境绑定，禁止迁移，这在某些场景下很有用，如使用线程局部存储的场景。
- 返回值获取

	协程的返回值获取非常简单易用。
- 协程局部存储

	方便的协程局部存储，让每个协程拥有自己的私有数据。



## 编译安装

### 安装xmake

此工程使用xmake管理，所以构建需要安装xmake，安装方法：

- 通过curl

```Bash
bash <(curl -fsSL https://xmake.io/shget.text)
```


- 通过wget

```Bash
bash <(wget https://xmake.io/shget.text -O -)
```


### 安装cmake

如果需要编译运行测试用例，需要依赖cmake。

### 拉取代码

```Git
git clone https://github.com/skyfireitdiy/cocpp
```


如果需要编译运行测试程序，需要将gtest和mockcpp也clone下来，这两个工程作为cocpp的子模块，可以直接使用如下命令：

```Git
git clone --recursive https://github.com/skyfireitdiy/cocpp
```


或者如果已经clone过了，可以在源码目录下使用如下命令更新子模块：

```Git
git submodule init
git submodule update
```


### 编译

#### cocpp

在源码目录，使用以下命令编译cocpp：

```Bash
xmake f --mode=release # 设置编译模式为release，也可以设置为debug
xmake b cocpp # 编译cocpp
```


#### test

如果需要运行测试用例，需要先编译gtest与mockcpp：

```Bash
./build_3rd.sh
```


然后编译运行test:

```Bash
xmake f --mode=release # 设置编译模式为release，也可以设置为debug
xmake b test # 编译并运行cocpp

```


### 安装

使用如下命令可以安装至/usr/local/目录（需要root权限）：

```Bash
xmake install cocpp
```


或者也可以指定安装目录：

```Bash
xmake install -o /install/path cocpp
```


## 使用示例

使用的例子参见example目录。

## 联系作者

- E-mail : skyfireitdiy@hotmail.com
- QQ: 1513008876

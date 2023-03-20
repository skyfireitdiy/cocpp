# README

## Introduction

cocpp is a coroutine library based on modern C++ (c++20). Its main features are easy to use, good encapsulation, and low learning cost. If you can use std::thread, you can use cocpp.

The current environment requirements are:

- x86_64 architecture
- Linux operating system
- gcc supports C++20

### hello world

The simplest demo:

```C++
#include "cocpp/cocpp.h"

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


## Feature introduction

The current features supported by cocpp are as follows:

- Free coroutine entry function

	Any style of function can be used as the function signature, whether it has a return value, no return value, no parameters, one parameter, or multiple parameters, it can be used as the coroutine entry function.
- Coroutine priority

	The priority of the coroutine can be specified when the coroutine is created, and the priority can also be modified during the coroutine running process (priority 0~99, where 0 is the highest and 99 is the lowest, and the default priority of the created coroutine is 99). Coroutines with different priorities are scheduled first with high-priority coroutines, and coroutines with the same priority are polled and scheduled.
- Multi-threaded load balancing

	Coroutines can be migrated between different threads to achieve load balancing.
- Dead loop forced scheduling

	When the coroutine environment has not been scheduled for more than the set threshold, it will be forcibly scheduled from the outside.
- System call timeout, reassign coroutine

	When a coroutine is blocked due to a system call, the manager will migrate all other migratable coroutines on the current thread to other suitable execution environments.
- Coroutine stealing

	When the current thread is idle, it will try to steal coroutines from other execution environments to execute.
- Coroutine synchronization tools

	It has rich coroutine synchronization tools. Semaphore, binary semaphore, conditional variable, mutex, recursive lock, read-write lock, etc.
- Channel for communication

	Like Go, it has channels for coroutine communication.

- Shared stack

	Multiple coroutines can share a stack space to reduce memory usage (shared stack coroutines do not support migration).
- Running environment binding

	Coroutines can be bound to a certain running environment to prohibit migration, which is very useful in some scenarios, such as using thread-local storage.
- Return value acquisition

	The return value of the coroutine is very simple and easy to use.
- Coroutine local storage

	Convenient coroutine local storage, allowing each coroutine to have its own private data.

- Support for parallel data processing mode with pipeline

```cpp
#include "cocpp/cocpp.h"
#include <iostream>

using namespace cocpp;
using namespace std;

int main()
{
    int source = 0;
    auto ch = co_pipeline<int>([&source]() -> std::optional<int> { // Generate [0, 1000000) sequence
                  if (source < 1000000)
                  {
                      return source++;
                  }
                  return std::nullopt;
              })
              | pipeline::take(2000) // Take the first 2000 items
              | pipeline::skip(1000) // Skip the first 1000 items (equivalent to taking [1000, 1999))
              | pipeline::fork(10, [](int n) -> int { // Multiply each number by 2, 10 coroutines calculate at the same time
                    return n * 2;
                })
              | pipeline::filter([](int n) { return n % 3 == 0; })  // Filter out multiples of 3
              | pipeline::reduce([](int n, int m) { return n + m; }, 0) // Sum
              | pipeline::chan(); // Convert to channel

    for (auto t : ch)
    {
        cout << t << endl;
    }
}
```

## Compilation and installation

### Install xmake

This project uses xmake management, so building requires xmake installation, installation method:

- Through curl

```Bash
bash <(curl -fsSL https://xmake.io/shget.text)
```


- Through wget

```Bash
bash <(wget https://xmake.io/shget.text -O -)
```


### Install cmake

If you need to compile and run test cases, you need to rely on cmake.

### Pull code

```Git
git clone https://github.com/skyfireitdiy/cocpp
```


If you need to compile and run the test program, you need to clone gtest and mockcpp as well. These two projects are submodules of cocpp and can be used directly with the following command:

```Git
git clone --recursive https://github.com/skyfireitdiy/cocpp
```


Or if you have already cloned, you can use the following command to update the submodule in the source directory:

```Git
git submodule init
git submodule update
```


### Compilation

#### cocpp

In the source directory, use the following command to compile cocpp:

```Bash
xmake f --mode=release # Set the compilation mode to release, you can also set it to debug
xmake b cocpp # Compile cocpp
```

#### test

If you need to run the test case, you need to compile gtest and mockcpp first:

```Bash
./build_3rd.sh
```


Then compile and run the test:

```Bash
xmake f --mode=release # Set the compilation mode to release, you can also set it to debug
xmake b test # Compile and run cocpp
```


### Installation

Use the following command to install to the /usr/local/ directory (requires root privileges):

```Bash
xmake install cocpp
```


Or you can also specify the installation directory:

```Bash
xmake install -o /install/path cocpp
```


## Example

The example used is in the example directory.

## Contact the author

- E-mail : skyfireitdiy@hotmail.com
- QQ: 1513008876

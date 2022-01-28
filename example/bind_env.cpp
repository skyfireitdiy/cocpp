// 此example演示绑定env
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/interface/co.h"
#include "cocpp/interface/co_this_co.h"

#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;

void print_thread_id()
{
    cocpp::this_co::sleep_for(1s);
    printf("thread id: %d\n", ::gettid());
}

int main()
{
    // 这里使用true表示不会被自动回收，用于绑定的env一定要设置为true
    auto      env = cocpp::co::create_env(true);
    cocpp::co c1({ cocpp::with_bind_env(env) }, &print_thread_id);
    cocpp::co c2({ cocpp::with_bind_env(env) }, &print_thread_id);
    cocpp::co c3({ cocpp::with_bind_env(env) }, &print_thread_id);
    cocpp::co c4({ cocpp::with_bind_env(env) }, &print_thread_id);

    c1.wait<void>();
    c2.wait<void>();
    c3.wait<void>();
    c4.wait<void>();
}
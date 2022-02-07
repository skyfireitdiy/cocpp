// 此 example 演示接受不同函数签名的可调用对象

#include "cocpp/interface/co.h"
#include "cocpp/interface/co_async_run.h"
#include <cstdio>
#include <functional>

int add(int a, int b)
{
    return a + b;
}

struct sub
{
    int operator()(int a, int b)
    {
        return a - b;
    }
};

auto mul = [](int a, int b) { return a * b; };

int div_(int a, int b)
{
    return a / b;
}

int main()
{
    auto add_co = cocpp::co_async_run(&add, 1000, 200);
    auto sub_co = cocpp::co_async_run(sub {}, 1000, 200);
    auto mul_co = cocpp::co_async_run(mul, 1000, 200);
    auto p_div  = std::function<int(int, int)>(&div_);
    auto div_co = cocpp::co_async_run(p_div, 1000, 200);

    auto add_ret = add_co.get();
    auto sub_ret = sub_co.get();
    auto mul_ret = mul_co.get();
    auto div_ret = div_co.get();

    printf("add: %d\n", add_ret);
    printf("sub: %d\n", sub_ret);
    printf("mul: %d\n", mul_ret);
    printf("div: %d\n", div_ret);
}
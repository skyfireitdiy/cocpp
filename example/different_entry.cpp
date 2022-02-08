// 此 example 演示接受不同函数签名的可调用对象

#include "cocpp/interface/co.h"

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
    cocpp::co add_co(&add, 1000, 200);
    cocpp::co sub_co(sub {}, 1000, 200);
    cocpp::co mul_co(mul, 1000, 200);
    auto      p_div = std::function<int(int, int)>(&div_);
    cocpp::co div_co(p_div, 1000, 200);

    auto add_ret = add_co.wait<int>();
    auto sub_ret = sub_co.wait<int>();
    auto mul_ret = mul_co.wait<int>();
    auto div_ret = div_co.wait<int>();

    printf("add: %d\n", add_ret);
    printf("sub: %d\n", sub_ret);
    printf("mul: %d\n", mul_ret);
    printf("div: %d\n", div_ret);
}
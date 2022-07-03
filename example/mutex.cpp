// 此 example 演示co_mutex使用

#include "cocpp/cocpp.h"

#include <cstdio>

long long       value = 0;
cocpp::co_mutex mu;

void add_value()
{
    printf("add co id: %lld\n", cocpp::this_co::id());
    for (long long i = 0; i < 10000; ++i)
    {
        std::scoped_lock lck(mu);
        value += i;
    }
}

void sub_value()
{
    printf("sub co id: %lld\n", cocpp::this_co::id());
    for (long long i = 0; i < 10000; ++i)
    {
        std::scoped_lock lck(mu);
        value -= i;
    }
}

int main()
{
    cocpp::co co_add(&add_value);
    cocpp::co co_sub(&sub_value);
    co_add.join();
    co_sub.join();
    printf("value: %lld\n", value);
}
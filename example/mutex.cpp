// 此 example 演示co_mutex使用

#include "cocpp/interface/co.h"
#include "cocpp/interface/co_this_co.h"
#include "cocpp/sync/co_mutex.h"

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
    co_add.wait<void>();
    co_sub.wait<void>();
    printf("value: %lld\n", value);
}
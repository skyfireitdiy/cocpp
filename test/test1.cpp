#include "co.h"
#include "co_default_manager.h"
#include <functional>
#include <iostream>
#include <thread>

void func(int n)
{
    for (int i = 0; i < 10000; ++i)
    {
        printf("%d\n", n);
        co::schedule_switch();
    }
}

int add(int a, int b)
{
    return a + b;
}

void add2(int a, int b)
{
    printf("%d + %d = %d\n", a, b, a + b);
}

void test1()
{
    co c1(std::function(func), 1);
    co c2(std::function(func), 2);

    c1.wait<void>();
    c2.wait<void>();

    printf("finished\n");
}

void test2()
{
    co  c1(std::function(add), 1, 2);
    int ret = c1.wait<int>();
    printf("ret = %d\n", ret);
}

void test3()
{
    co c1(std::function(add2), 1, 2);
    c1.wait<void>();
}

int main()
{
    co::init_co(co_default_manager::instance());

    CO_DEBUG("thread %d", std::this_thread::get_id());

    test3();

    getchar();

    co::uninit_co();
}
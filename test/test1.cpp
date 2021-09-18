#include "co.h"
#include "co_default_manager.h"
#include <chrono>
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
    co c1(std::function<void(int)>(func), 1);
    co c2(std::function<void(int)>(func), 2);

    c1.wait<void>();
    c2.wait<void>();

    printf("finished\n");
}

void test2()
{
    co  c1(std::function<int(int, int)>(add), 1, 2);
    int ret = c1.wait<int>();
    printf("ret = %d\n", ret);
}

void test3()
{
    co c1(std::function<void(int, int)>(add2), 1, 2);
    c1.wait<void>();
}

void test4()
{
    co c1([]() {
        for (;;)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            printf("hello\n");
            co::schedule_switch();
        }
    });
    getchar();
    c1.detach();
}

void test5()
{
    co c1([]() {
        printf("%s %llu\n", co::name().c_str(), co::id());
    });
    printf("%s %llu\n", co::name().c_str(), co::id());
    c1.wait<void>();
}

void test6()
{
    std::thread([]() {
        printf("this is thread context: %d\n", std::this_thread::get_id());
        co::convert_to_schedule_thread();
    }).detach();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    co c1([]() {
        printf("this is co context, thread: %d\n", std::this_thread::get_id());
    });

    c1.wait<void>();
}

void test7()
{
    auto f = []() {
        for (int i = 0; i < 10; ++i)
        {
            co::sleep_for(std::chrono::seconds(1));
            printf("co %s -> %d\n", co::name(), i);
            co::schedule_switch();
        }
    };
    co c1({ with_priority(1), with_name("c1") }, f);
    co c2({ with_priority(1), with_name("c2") }, f);
    co c3({ with_priority(5), with_name("c3") }, f);
    co c4({ with_priority(6), with_name("c4") }, f);

    c1.wait<void>();
    c2.wait<void>();
    c3.wait<void>();
    c4.wait<void>();
}

bool test8_flag = true;

void test8()
{
    co c1({ with_name("c1") }, []() {
        for (int i = 0; i < 1000; ++i)
        {
            // printf("co1 %d\n", i);
            co::schedule_switch();
        }
    });
    co c2({ with_name("c2") }, []() {
        for (int i = 0; i < 1000; ++i)
        {
            co::schedule_switch();
        }
    });
    c1.wait<void>();
    c2.wait<void>();
}

int main()
{
    co::init_co(co_default_manager::instance());

    CO_DEBUG("thread %u", std::this_thread::get_id());

    test8();

    getchar();

    test8_flag = false;

    co::uninit_co();
}
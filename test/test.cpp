#if 0

#include "co.h"
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#else

#include "co.h"
#include "co_this_co.h"
#include <chrono>
#include <cstdio>
#include <thread>
#include <unistd.h>
using namespace cocpp;
using namespace std;

int main()
{
    auto env = co::create_env(true);
    auto f   = [](int n) {
        while (true)
        {
            this_thread::sleep_for(chrono::seconds(1));
            printf("this thread: %d, this is %s(0x%llx), I have %d\n", ::gettid(), this_co::name().c_str(), this_co::id(), n);
        }
    };
    co c1({ with_bind_env(env), with_name("c1") }, f, 1);
    co c2({ with_bind_env(env), with_name("c2") }, f, 2);

    getchar();
}

#endif
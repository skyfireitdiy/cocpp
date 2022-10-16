#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <unistd.h>
#include <vector>
#define private public
#include "cocpp/cocpp.h"

using namespace cocpp;
using namespace std;
using namespace chrono_literals;

TEST(stl, vector)
{
    vector<int> v1;
    vector<int> v2;
    co c1([&] {
        for (auto i = 0; i < 10000; ++i)
        {
            v1.push_back(i);
            this_co::yield();
        }
    });
    co c2([&] {
        for (auto i = 0; i < 10000; ++i)
        {
            v2.push_back(i);
            this_co::yield();
        }
    });
    c1.join();
    c2.join();
    EXPECT_EQ(v1, v2);
}

TEST(any, co_any)
{
    co_any any(5);
    EXPECT_EQ(any.get<int>(), 5);
}
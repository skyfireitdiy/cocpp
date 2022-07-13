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

TEST(chan, buffered_order)
{
    co_chan<int> ch(5);
    ch.push(1);
    ch.push(2);
    ch.push(3);
    ch.push(4);
    ch.push(5);
    ch.close();
    EXPECT_EQ(ch.pop(), 1);
    EXPECT_EQ(ch.pop(), 2);
    EXPECT_EQ(ch.pop(), 3);
    EXPECT_EQ(ch.pop(), 4);
    EXPECT_EQ(ch.pop(), 5);
    EXPECT_FALSE(ch.pop());
}

TEST(chan, buffered)
{
    co_chan<int> ch(10);

    // auto env = co::create_env(true);

    co c1([&] {
        for (int i = 0; i < 1000; ++i)
        {
            ch.push(i);
        }
        ch.close();
    });

    for (int i = 0; i < 1000; ++i)
    {
        EXPECT_EQ(ch.pop(), i);
    }
    EXPECT_FALSE(ch.pop());

    c1.join();
}

TEST(chan, no_limited)
{
    co_chan<int> ch(-1);

    co  c1([&] {
        for (int i = 0; i < 10000; ++i)
        {
            ch.push(i);
        }
        ch.close();
    });
    int t;
    for (int i = 0; i < 10000; ++i)
    {
        t = ch.pop().value();
        EXPECT_EQ(t, i);
    }
    EXPECT_FALSE(ch.pop());
    c1.join();
}

TEST(chan, no_buf)
{
    co_chan<int> ch(0);
    co           c1([&] {
        for (int i = 0; i < 1000; ++i)
        {
            ch << i;
        }
        ch.close();
    });

    for (int i = 0; i < 1000; ++i)
    {
        int t = ch.pop().value();
        EXPECT_EQ(t, i);
    }
    EXPECT_FALSE(ch.pop());
    c1.join();
}

TEST(chan, no_buffered_iterator)
{
    co_chan<int> ch(0);

    vector<int> push { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    vector<int> pop;

    co c1([&] {
        for (auto&& p : push)
        {
            ch.push(p);
        }
        ch.close();
    });

    for (auto&& p : ch)
    {
        pop.push_back(p);
    }
    c1.join();
    EXPECT_EQ(push, pop);
}

TEST(chan, buffered_empty_iterator)
{
    co_chan<int> ch(10);
    vector<int>  pop;
    vector<int>  push;

    ch.close();

    for (auto&& p : ch)
    {
        pop.push_back(p);
    }
    EXPECT_EQ(pop, push);
}

TEST(chan, no_buffered_operator_less)
{
    co_chan<int> ch(0);

    vector<int> push { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    vector<int> pop;

    co c1([&] {
        for (auto&& p : push)
        {
            [[maybe_unused]] auto ret = ch < p;
        }
        ch.close();
    });

    while (true)
    {
        int n;
        if (ch > n)
        {
            pop.push_back(n);
        }
        else
        {
            break;
        }
    }

    c1.join();
    EXPECT_EQ(push, pop);
}

TEST(chan, no_buffered_operator_shift)
{
    co_chan<int> ch(0);

    vector<int> pop;

    co c1([&] {
        ch << 0 << 1 << 2 << 3 << 4;
        ch.close();
    });

    int a, b, c, d, e;
    ch >> a >> b >> c >> d >> e;

    c1.join();
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 1);
    EXPECT_EQ(c, 2);
    EXPECT_EQ(d, 3);
    EXPECT_EQ(e, 4);

    EXPECT_ANY_THROW(ch >> a);
}

TEST(chan, push_to_closed)
{
    co_chan<int> ch(1);
    ch.close();
    EXPECT_FALSE(ch.push(1));
}

TEST(chan, push_to_full_chan_closed)
{
    co_chan<int> ch(1);
    ch.push(1);
    co c1([&] {
        this_co::sleep_for(100ms);
        ch.close();
    });
    EXPECT_FALSE(ch.push(1));
    c1.join();
}

TEST(chan, pop_from_empty_chan_closed)
{
    co_chan<int> ch(1);
    co           c1([&] {
        this_co::sleep_for(100ms);
        ch.close();
    });
    EXPECT_FALSE(ch.pop());
    c1.join();
}

TEST(chan, pop_from_zero_chan_closed)
{
    co_chan<int> ch(0);
    co           c1([&] {
        this_co::sleep_for(100ms);
        ch.close();
    });
    EXPECT_FALSE(ch.pop());
    c1.join();
}

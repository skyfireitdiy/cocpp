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

TEST(sync, mutex_try_lock)
{
    co_mutex mu;

    co c1([&]() {
        scoped_lock lock(mu);
        this_co::sleep_for(100ms);
    });

    this_co::sleep_for(50ms);
    EXPECT_FALSE(mu.try_lock());
    c1.join();
}

TEST(sync, mutex_lock)
{
    co_mutex mu;

    int ret = 0;

    co c1([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            scoped_lock lock(mu);
            ret += i;
            this_co::yield();
        }
    });
    co c2([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            {
                scoped_lock lock(mu);
                ret += i;
            }
            this_co::yield();
        }
    });
    co c3([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            scoped_lock lock(mu);
            ret -= i;
            this_co::yield();
        }
    });
    for (int i = 0; i < 1000; ++i)
    {
        scoped_lock lock(mu);
        ret -= i;
        this_co::yield();
    }

    c1.join();
    c2.join();
    c3.join();

    EXPECT_EQ(ret, 0);
}

TEST(sync, mutex_throw)
{
    co_mutex mu;
    EXPECT_THROW(mu.unlock(), logic_error);
}

TEST(sync, recursive_mutex_lock)
{
    co_recursive_mutex mu;
    mu.lock();
    mu.lock();
    mu.lock();
    mu.unlock();
    mu.unlock();
    mu.unlock();
}

TEST(sync, recursive_mutex_trylock)
{
    co_recursive_mutex mu;
    EXPECT_TRUE(mu.try_lock());
    EXPECT_TRUE(mu.try_lock());
    EXPECT_TRUE(mu.try_lock());
    mu.unlock();
    mu.unlock();
    mu.unlock();
}

TEST(sync, timed_mutex_lock_for)
{
    co_timed_mutex mu;
    co c1([&] {
        mu.lock();
        this_co::sleep_for(100ms);
        mu.unlock();
    });
    this_co::sleep_for(50ms);
    EXPECT_FALSE(mu.try_lock_for(10ms));
    EXPECT_TRUE(mu.try_lock_for(60ms));
    mu.unlock();
    c1.join();
}

TEST(sync, shared_mutex_shared_lock)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock_shared();
        this_co::sleep_for(100ms);
        mu.unlock_shared();
    });
    this_co::sleep_for(50ms);
    EXPECT_TRUE(mu.try_lock_shared());
    mu.unlock_shared();
    c1.join();
}

TEST(sync, shared_mutex_try_lock_shared_locked_mutex)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock();
        this_co::sleep_for(100ms);
        mu.unlock();
    });
    this_co::sleep_for(50ms);
    EXPECT_FALSE(mu.try_lock_shared());
    c1.join();
}

TEST(sync, shared_mutex_lock_try_lock_shared_locked_mutex)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock_shared();
        this_co::sleep_for(100ms);
        mu.unlock_shared();
    });
    this_co::sleep_for(50ms);
    EXPECT_FALSE(mu.try_lock());
    c1.join();
}

TEST(sync, shared_mutex_lock_try_lock_locked_mutex)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock();
        this_co::sleep_for(100ms);
        mu.unlock();
    });
    this_co::sleep_for(50ms);
    EXPECT_FALSE(mu.try_lock());
    c1.join();
}

TEST(sync, shared_timed_mutex_try_lock_for_locked_shread_mutex)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock_shared();
        this_co::sleep_for(100ms);
        mu.unlock_shared();
    });
    this_co::sleep_for(50ms);
    EXPECT_FALSE(mu.try_lock_for(10ms));
    EXPECT_TRUE(mu.try_lock_for(60ms));
    mu.unlock();
    c1.join();
}

TEST(sync, shared_timed_mutex_try_lock_for_locked_mutex)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock();
        this_co::sleep_for(100ms);
        mu.unlock();
    });
    this_co::sleep_for(50ms);
    EXPECT_FALSE(mu.try_lock_for(10ms));
    EXPECT_TRUE(mu.try_lock_for(60ms));
    mu.unlock();
    c1.join();
}

TEST(sync, shared_timed_mutex_try_lock_shared_for_locked_mutex)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock();
        this_co::sleep_for(100ms);
        mu.unlock();
    });
    this_co::sleep_for(50ms);
    EXPECT_FALSE(mu.try_lock_shared_for(10ms));
    EXPECT_TRUE(mu.try_lock_shared_for(80ms));
    mu.unlock_shared();
    c1.join();
}

TEST(sync, condition_variable_notify_one)
{
    co_mutex mu;
    co_condition_variable cond;
    int n = 100;

    co c1([&] {
        unique_lock lck(mu);
        cond.wait(lck);
        n -= 5;
    });
    n /= 2;
    this_co::sleep_for(100ms);
    EXPECT_EQ(n, 50);
    cond.notify_one();
    c1.join();
    EXPECT_EQ(n, 45);
}

TEST(sync, condition_variable_notify_all)
{
    co_mutex mu;
    co_condition_variable cond;
    int n = 100;

    co c1([&] {
        unique_lock lck(mu);
        cond.wait(lck);
        n -= 5;
    });
    co c2([&] {
        unique_lock lck(mu);
        cond.wait(lck);
        n -= 10;
    });
    n /= 2;
    this_co::sleep_for(100ms);
    EXPECT_EQ(n, 50);
    cond.notify_all();
    c1.join();
    c2.join();
    EXPECT_EQ(n, 35);
}

TEST(sync, condition_variable_notify_at_co_exit)
{
    co_mutex mu;
    co_condition_variable cond;
    int n = 100;

    co c1([&] {
        notify_all_at_co_exit(cond);
        this_co::sleep_for(100ms);
        n /= 2;
    });
    c1.detach();
    EXPECT_EQ(n, 100);
    unique_lock lck(mu);
    cond.wait(lck);
    EXPECT_EQ(n, 50);
}

TEST(sync, call_once)
{
    co_once_flag flag;
    atomic<int> n = 0;

    auto f = [&](int t) {
        n += t;
    };

    co c1([&] {
        co_call_once(flag, f, 5);
    });
    co c2([&] {
        co_call_once(flag, f, 10);
    });

    c1.join();
    c2.join();
    EXPECT_TRUE(n == 5 || n == 10);
}

TEST(sync, counting_semaphore_normal)
{
    co_counting_semaphore<5> sem(0);

    co c1([&] {
        sem.acquire();
        sem.acquire();
        sem.acquire();
        sem.acquire();
        sem.acquire();
        sem.acquire();
        sem.acquire();
        sem.acquire();
        sem.acquire();
        sem.acquire();
        EXPECT_FALSE(sem.try_acquire());
        EXPECT_FALSE(sem.try_acquire_until(chrono::steady_clock::now() + 50ms));
    });
    sem.release(10);
    c1.join();
}

TEST(sync, barrier)
{
    atomic<int> n = 0;
    co_barrier barrier(10);
    vector<co> cs;

    for (int i = 0; i < 10; ++i)
    {
        cs.emplace_back([&] {
            ++n;
            barrier.arrive_and_wait();
            EXPECT_EQ(n, 10);
            barrier.arrive_and_wait();
            ++n;
            barrier.arrive_and_wait();
            EXPECT_EQ(n, 20);
        });
    }
}

TEST(sync, wait_group)
{
    atomic<int> n = 0;
    co_wait_group wg(10);
    vector<co> cs;

    for (int i = 0; i < 10; ++i)
    {
        cs.emplace_back([&](int begin, int end) {
            for (int i = begin; i < end; ++i)
            {
                n += i;
            }
            wg.done();
        },
                        i * 10, (i + 1) * 10);
    }
    wg.wait();
    EXPECT_EQ(n, 4950);
}

TEST(sync, latch)
{
    atomic<int> n = 0;
    co_latch latch(10);
    vector<co> cs;

    for (int i = 0; i < 10; ++i)
    {
        cs.emplace_back([&] {
            ++n;
            latch.arrive_and_wait();
            EXPECT_EQ(n, 10);
        });
    }
}
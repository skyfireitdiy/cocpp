#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <unistd.h>
#include <vector>
#define private public
#include "cocpp/comm/co_chan.h"
#include "cocpp/core/co_ctx_config.h"
#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_env_factory.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_stack_factory.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/interface/co.h"
#include "cocpp/interface/co_this_co.h"
#include "cocpp/mem/co_mem_pool.h"
#include "cocpp/sync/co_binary_semaphore.h"
#include "cocpp/sync/co_call_once.h"
#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/sync/co_counting_semaphore.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/sync/co_recursive_mutex.h"
#include "cocpp/sync/co_shared_mutex.h"
#include "cocpp/sync/co_shared_timed_mutex.h"
#include "cocpp/sync/co_timed_mutex.h"
#include "cocpp/utils/co_any.h"

using namespace cocpp;
using namespace std::chrono_literals;

TEST(co, name)
{
    co c1({ with_name("test1") }, []() {
        EXPECT_EQ(this_co::name(), "test1");
    });
    c1.wait<void>();
}

TEST(co, id)
{
    auto f = []() {
        for (int i = 0; i < 10; ++i)
        {
            printf("%s %llu %d\n", this_co::name().c_str(), this_co::id(), i);
            this_co::yield();
        }
    };
    co c1({ with_name("test1") }, f);
    co c2({ with_name("test1") }, f);
    c1.wait<void>();
    c2.wait<void>();
}

TEST(co, my_thread)
{
    std::thread th([]() {
        printf("new thread: %u\n", gettid());
        co::schedule_in_this_thread();
    });

    co c1([]() {
        printf("new co %llu in thread %u\n", this_co::id(), gettid());
    });

    c1.wait<void>();
    th.detach();
}

TEST(co, detach)
{
    co c1([]() {
        for (int i = 0; i < 100; ++i)
        {
            printf("count %d\n", i);
            this_co::yield();
        }
    });
    c1.detach();
}

TEST(co, ref)
{
    int t = 20;
    co  c1([](int& n) { n += 10; }, std::ref(t));
    c1.wait<void>();
    EXPECT_EQ(t, 30);
}

TEST(co, return_value)
{
    co c1([](int n) { return n + 10; }, 25);
    EXPECT_EQ(c1.wait<int>(), 35);
}

TEST(co, wait_timeout)
{
    co   c1([]() {
        this_co::sleep_for(1s);
    });
    auto ret = c1.wait(1ms);
    EXPECT_FALSE(ret);
    ret = c1.wait(10s);
    EXPECT_TRUE(ret);
}

TEST(co, priority)
{
    std::vector<int> arr;

    auto env = co::create_env(true);
    co   c1(
        { with_priority(0), with_bind_env(env) }, [](std::vector<int>& arr) {
            this_co::sleep_for(50ms);
            arr.push_back(100);
            this_co::yield();
            arr.push_back(200);
            this_co::yield();
            arr.push_back(300);
            this_co::yield();
        },
        std::ref(arr));
    co c2(
        { with_priority(1), with_bind_env(env) }, [](std::vector<int>& arr) {
            this_co::sleep_for(50ms);
            arr.push_back(400);
            this_co::yield();
            arr.push_back(500);
            this_co::yield();
            arr.push_back(600);
            this_co::yield();
        },
        std::ref(arr));
    c1.wait<void>();
    c2.wait<void>();

    std::vector<int> expect { 100, 200, 300, 400, 500, 600 };
    EXPECT_EQ(arr, expect);
}

TEST(co, co_id)
{
    co_id id;
    co    c1([&id]() {
        id = this_co::id();
    });
    c1.wait<void>();
    EXPECT_EQ(c1.id(), id);
}

TEST(co, co_id_name_after_detach)
{
    co c1([]() {
    });
    c1.wait<void>();
    c1.detach();
    EXPECT_EQ(c1.id(), 0ULL);
    EXPECT_EQ(c1.name(), "");
}

TEST(co, other_co_name)
{
    co c1({ with_name("zhangsan") }, []() {});
    c1.wait<void>();
    EXPECT_EQ(c1.name(), "zhangsan");
}

TEST(co, co_mutex_try_lock)
{
    co_mutex mu;

    co c1([&]() {
        std::scoped_lock lock(mu);
        this_co::sleep_for(1s);
    });

    this_co::sleep_for(500ms);
    EXPECT_FALSE(mu.try_lock());
    c1.wait<void>();
}

TEST(co, co_mutex_lock)
{
    co_mutex mu;

    int ret = 0;

    co c1([&]() {
        for (int i = 0; i < 100000; ++i)
        {
            std::scoped_lock lock(mu);
            ret += i;
            this_co::yield();
        }
    });
    co c2([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            {
                std::scoped_lock lock(mu);
                ret += i;
            }
            this_co::yield();
        }
    });
    co c3([&]() {
        for (int i = 0; i < 100000; ++i)
        {
            std::scoped_lock lock(mu);
            ret -= i;
            this_co::yield();
        }
    });
    for (int i = 0; i < 1000; ++i)
    {
        std::scoped_lock lock(mu);
        ret -= i;
        this_co::yield();
    }

    c1.wait<void>();
    c2.wait<void>();
    c3.wait<void>();

    EXPECT_EQ(ret, 0);
}

TEST(co, co_mutex_throw)
{
    co_mutex mu;
    EXPECT_THROW(mu.unlock(), co_error);
}

TEST(co, co_recursive_mutex_lock)
{
    co_recursive_mutex mu;
    mu.lock();
    mu.lock();
    mu.lock();
    mu.unlock();
    mu.unlock();
    mu.unlock();
}

TEST(co, co_recursive_mutex_trylock)
{
    co_recursive_mutex mu;
    EXPECT_TRUE(mu.try_lock());
    EXPECT_TRUE(mu.try_lock());
    EXPECT_TRUE(mu.try_lock());
    mu.unlock();
    mu.unlock();
    mu.unlock();
}

TEST(co, co_timed_mutex)
{
    co_timed_mutex mu;
    co             c1([&] {
        mu.lock();
        this_co::sleep_for(1s);
        mu.unlock();
    });
    this_co::sleep_for(500ms);
    EXPECT_FALSE(mu.try_lock_for(100ms));
    EXPECT_TRUE(mu.try_lock_for(600ms));
    mu.unlock();
    c1.wait<void>();
}

TEST(co, co_shared_mutex_shared_lock)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock_shared();
        this_co::sleep_for(1s);
        mu.unlock_shared();
    });
    this_co::sleep_for(500ms);
    EXPECT_TRUE(mu.try_lock_shared());
    mu.unlock_shared();
    c1.wait<void>();
}

TEST(co, co_shared_mutex_lock)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock();
        this_co::sleep_for(1s);
        mu.unlock();
    });
    this_co::sleep_for(500ms);
    EXPECT_FALSE(mu.try_lock_shared());
    c1.wait<void>();
}

TEST(co, co_shared_mutex_lock2)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock_shared();
        this_co::sleep_for(1s);
        mu.unlock_shared();
    });
    this_co::sleep_for(500ms);
    EXPECT_FALSE(mu.try_lock());
    c1.wait<void>();
}

TEST(co, co_shared_mutex_lock3)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock();
        this_co::sleep_for(1s);
        mu.unlock();
    });
    this_co::sleep_for(500ms);
    EXPECT_FALSE(mu.try_lock());
    c1.wait<void>();
}

TEST(co, co_shared_timed_mutex1)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock_shared();
        this_co::sleep_for(1s);
        mu.unlock_shared();
    });
    this_co::sleep_for(500ms);
    EXPECT_FALSE(mu.try_lock_for(100ms));
    EXPECT_TRUE(mu.try_lock_for(600ms));
    mu.unlock();
    c1.wait<void>();
}

TEST(co, co_shared_timed_mutex2)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock();
        this_co::sleep_for(1s);
        mu.unlock();
    });
    this_co::sleep_for(500ms);
    EXPECT_FALSE(mu.try_lock_for(100ms));
    EXPECT_TRUE(mu.try_lock_for(600ms));
    mu.unlock();
    c1.wait<void>();
}

TEST(co, co_shared_timed_mutex3)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock();
        this_co::sleep_for(1s);
        mu.unlock();
    });
    this_co::sleep_for(500ms);
    EXPECT_FALSE(mu.try_lock_shared_for(100ms));
    EXPECT_TRUE(mu.try_lock_shared_for(800ms));
    mu.unlock_shared();
    c1.wait<void>();
}

TEST(co, co_condition_variable_notify_one)
{
    co_mutex              mu;
    co_condition_variable cond;
    int                   n = 100;

    co c1([&] {
        std::unique_lock lck(mu);
        cond.wait(lck);
        n -= 5;
    });
    n /= 2;
    this_co::sleep_for(1s);
    EXPECT_EQ(n, 50);
    cond.notify_one();
    c1.wait<void>();
    EXPECT_EQ(n, 45);
}

TEST(co, co_condition_variable_notify_all)
{
    co_mutex              mu;
    co_condition_variable cond;
    int                   n = 100;

    co c1([&] {
        std::unique_lock lck(mu);
        cond.wait(lck);
        n -= 5;
    });
    co c2([&] {
        std::unique_lock lck(mu);
        cond.wait(lck);
        n -= 10;
    });
    n /= 2;
    this_co::sleep_for(1s);
    EXPECT_EQ(n, 50);
    cond.notify_all();
    c1.wait<void>();
    c2.wait<void>();
    EXPECT_EQ(n, 35);
}

TEST(co, co_condition_variable_notify_at_co_exit)
{
    co_mutex              mu;
    co_condition_variable cond;
    int                   n = 100;

    co c1([&] {
        notify_all_at_co_exit(cond);
        this_co::sleep_for(1s);
        n /= 2;
    });
    c1.detach();
    EXPECT_EQ(n, 100);
    std::unique_lock lck(mu);
    cond.wait(lck);
    EXPECT_EQ(n, 50);
}

TEST(co, co_call_once)
{
    co_once_flag     flag;
    std::atomic<int> n = 0;

    auto f = [&](int t) {
        n += t;
    };

    co c1([&] {
        co_call_once(flag, f, 5);
    });
    co c2([&] {
        co_call_once(flag, f, 10);
    });

    c1.wait<void>();
    c2.wait<void>();
    EXPECT_TRUE(n == 5 || n == 10);
}

TEST(co, co_counting_semaphore_normal)
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
        EXPECT_FALSE(sem.try_acquire_until(std::chrono::steady_clock::now() + 50ms));
    });
    sem.release(10);
    c1.wait<void>();
}

TEST(co, co_chan_buffered)
{
    co_chan<int, 5> ch;
    ch.push(1);
    ch.push(2);
    ch.push(3);
    ch.push(4);
    ch.push(5);
}

TEST(co, co_chan_buffered2)
{
    co_chan<int, 2> ch;

    co  c1([&] {
        for (int i = 0; i < 10; ++i)
        {
            ch.push(i);
        }
        ch.close();
    });
    int t;
    for (int i = 0; i < 10; ++i)
    {
        t = ch.pop().value();
        EXPECT_EQ(t, i);
    }
    EXPECT_FALSE(ch.pop());
    c1.wait<void>();
}

TEST(co, co_chan_no_buffered)
{
    co_chan<int, 0> ch;

    co  c1([&] {
        for (int i = 0; i < 10; ++i)
        {
            ch.push(i);
        }
        ch.close();
    });
    int t;
    for (int i = 0; i < 10; ++i)
    {
        t = ch.pop().value();
        EXPECT_EQ(t, i);
    }
    EXPECT_FALSE(ch.pop());
    c1.wait<void>();
}

TEST(co, co_chan_no_buffered_iterator)
{
    co_chan<int, 0> ch;

    std::vector<int> push { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<int> pop;

    co c1([&] {
        for (auto p : push)
        {
            ch.push(p);
        }
        ch.close();
    });

    for (auto& p : ch)
    {
        pop.push_back(p);
    }
    c1.wait<void>();
    EXPECT_EQ(push, pop);
}

TEST(co, co_chan_buffered_empty_iterator)
{
    co_chan<int, 10> ch;
    std::vector<int> pop;
    std::vector<int> push;

    ch.close();

    for (auto& p : ch)
    {
        pop.push_back(p);
    }
    EXPECT_EQ(pop, push);
}

TEST(co, co_chan_no_buffered_operator_less)
{
    co_chan<int, 0> ch;

    std::vector<int> push { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<int> pop;

    co c1([&] {
        for (auto p : push)
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

    c1.wait<void>();
    EXPECT_EQ(push, pop);
}

TEST(co, co_chan_no_buffered_operator_shift)
{
    co_chan<int, 0> ch;

    std::vector<int> pop;

    co c1([&] {
        ch << 0 << 1 << 2 << 3 << 4;
        ch.close();
    });

    int a, b, c, d, e;
    ch >> a >> b >> c >> d >> e;

    c1.wait<void>();
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 1);
    EXPECT_EQ(c, 2);
    EXPECT_EQ(d, 3);
    EXPECT_EQ(e, 4);

    EXPECT_ANY_THROW(ch >> a);
}

TEST(co, co_wait_priority)
{
    auto env = co::create_env(true);

    co c1({ with_priority(99), with_bind_env(env) }, [] {
        this_co::sleep_for(1s);
    });

    co c2({ with_priority(0), with_bind_env(env) }, [&] {
        c1.wait<void>();
    });

    c2.wait<void>();
}

TEST(co, co_shared_stack)
{
    auto env = co::create_env(true);

    auto routine = [](int n) {
        int sum = 0;
        for (int i = 0; i < 1000; ++i)
        {
            sum += i;
            this_co::yield();
        }
        return sum;
    };
    co c1({ with_name("shared stack ctx 1"), with_shared_stack(true), with_bind_env(env) }, routine, 1);
    co c2({ with_name("shared stack ctx 2"), with_shared_stack(true), with_bind_env(env) }, routine, 2);

    EXPECT_EQ(c1.wait<int>(), 499500);
    EXPECT_EQ(c2.wait<int>(), 499500);
}

TEST(co, co_local)
{
    co c1([]() {
        CoLocal(name, std::string) = "hello";
        auto& value                = CoLocal(name, std::string);
        EXPECT_EQ(value, "hello");
    });

    c1.wait<void>();
    auto& value = CoLocal(name, std::string);
    EXPECT_EQ(value, "");
}

TEST(co, co_chan_push_to_closed)
{
    co_chan<int, 1> ch;
    ch.close();
    EXPECT_FALSE(ch.push(1));
}

TEST(co, co_chan_push_to_full_chan_closed)
{
    co_chan<int, 1> ch;
    ch.push(1);
    co c1([&] {
        this_co::sleep_for(1s);
        ch.close();
    });
    EXPECT_FALSE(ch.push(1));
    c1.wait<void>();
}

TEST(co, co_chan_pop_from_empty_chan_closed)
{
    co_chan<int, 1> ch;
    co              c1([&] {
        this_co::sleep_for(1s);
        ch.close();
    });
    EXPECT_FALSE(ch.pop());
    c1.wait<void>();
}

TEST(co, co_chan_pop_from_zero_chan_closed)
{
    co_chan<int, 0> ch;
    co              c1([&] {
        this_co::sleep_for(1s);
        ch.close();
    });
    EXPECT_FALSE(ch.pop());
    c1.wait<void>();
}

TEST(co, zone_edge)
{
    EXPECT_EQ(co_mem_pool::align_2_zone_edge__(1), 0ULL);
    EXPECT_EQ(co_mem_pool::align_2_zone_edge__(2), 1ULL);
    EXPECT_EQ(co_mem_pool::align_2_zone_edge__(3), 2ULL);
    EXPECT_EQ(co_mem_pool::align_2_zone_edge__(4), 2ULL);
}

TEST(stl, vector)
{
    std::vector<std::string> v1;
    std::vector<std::string> v2;
    co                       c1([&] {
        for (auto i = 0; i < 1000000; ++i)
        {
            v1.push_back(std::to_string(i));
            this_co::yield();
        }
    });
    co                       c2([&] {
        for (auto i = 0; i < 1000000; ++i)
        {
            v2.push_back(std::to_string(i));
            this_co::yield();
        }
    });
    c1.wait<void>();
    c2.wait<void>();
    EXPECT_EQ(v1, v2);
}

TEST(co, co_any)
{
    co_any any(5);
    EXPECT_EQ(any.get<int>(), 5);
}
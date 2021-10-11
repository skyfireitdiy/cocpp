#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <mutex>
#define private public
#include "co.h"
#include "co_binary_semaphore.h"
#include "co_call_once.h"
#include "co_condition_variable.h"
#include "co_counting_semaphore.h"
#include "co_ctx_factory.h"
#include "co_env_factory.h"
#include "co_error.h"
#include "co_manager.h"
#include "co_mutex.h"
#include "co_o1_scheduler_factory.h"
#include "co_recursive_mutex.h"
#include "co_shared_mutex.h"
#include "co_shared_timed_mutex.h"
#include "co_stack_factory.h"
#include "co_timed_mutex.h"

using namespace cocpp;

TEST(co, name)
{
    co c1({ with_name("test1") }, [this]() {
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
        co::convert_to_schedule_thread();
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
        co::sleep_for(std::chrono::seconds(1));
    });
    auto ret = c1.wait(std::chrono::milliseconds(1));
    EXPECT_FALSE(ret);
    ret = c1.wait(std::chrono::milliseconds(10000));
    EXPECT_TRUE(ret);
}

// 两个协程可能运行在不同的线程上，所以此处的优先级不起作用
// TEST(co, priority)
// {
//     std::vector<int> arr;
//     co               c1(
//         { with_priority(0) }, [](std::vector<int>& arr) {
//             co::sleep_for(std::chrono::milliseconds(50));
//             arr.push_back(100);
//             this_co::yield()();
//             arr.push_back(200);
//             this_co::yield()();
//             arr.push_back(300);
//             this_co::yield()();
//         },
//         std::ref(arr));
//     co c2(
//         { with_priority(1) }, [](std::vector<int>& arr) {
//             co::sleep_for(std::chrono::milliseconds(50));
//             arr.push_back(400);
//             this_co::yield()();
//             arr.push_back(500);
//             this_co::yield()();
//             arr.push_back(600);
//             this_co::yield()();
//         },
//         std::ref(arr));
//     c1.wait<void>();
//     c2.wait<void>();
//     std::vector<int> expect { 100, 200, 300, 400, 500, 600 };
//     EXPECT_EQ(arr, expect);
// }

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
    co_id id;
    co    c1([&id]() {
    });
    c1.wait<void>();
    c1.detach();
    EXPECT_EQ(c1.id(), 0);
    EXPECT_EQ(c1.name(), "");
}

TEST(co, other_co_name)
{
    co c1({ with_name("zhangsan") }, []() {});
    c1.wait<void>();
    EXPECT_EQ(c1.name(), "zhangsan");
}

TEST(co, co_spinlock_try_lock)
{
    co_spinlock mu;

    co c1([&]() {
        std::lock_guard<co_spinlock> lock(mu);
        co::sleep_for(std::chrono::seconds(1));
    });

    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock());
    c1.wait<void>();
}

TEST(co, co_spinlock_lock)
{
    co_spinlock mu;

    int ret = 0;

    co c1([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            std::lock_guard<co_spinlock> lock(mu);
            ret += i;
        }
    });
    for (int i = 0; i < 1000; ++i)
    {
        std::lock_guard<co_spinlock> lock(mu);
        ret -= i;
    }

    c1.wait<void>();

    EXPECT_EQ(ret, 0);
}

TEST(co, co_mutex_try_lock)
{
    co_mutex mu;

    co c1([&]() {
        std::lock_guard<co_mutex> lock(mu);
        co::sleep_for(std::chrono::seconds(1));
    });

    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock());
    c1.wait<void>();
}

TEST(co, co_mutex_lock)
{
    co_mutex mu;

    int ret = 0;

    co c1([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            std::lock_guard<co_mutex> lock(mu);
            ret += i;
            this_co::yield();
        }
    });
    co c2([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            std::lock_guard<co_mutex> lock(mu);
            ret += i;
            this_co::yield();
        }
    });
    co c3([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            std::lock_guard<co_mutex> lock(mu);
            ret -= i;
            this_co::yield();
        }
    });
    for (int i = 0; i < 1000; ++i)
    {
        std::lock_guard<co_mutex> lock(mu);
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
        co::sleep_for(std::chrono::seconds(1));
        mu.unlock();
    });
    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock_for(std::chrono::milliseconds(100)));
    EXPECT_TRUE(mu.try_lock_for(std::chrono::milliseconds(600)));
    mu.unlock();
    c1.wait<void>();
}

TEST(co, co_shared_mutex_shared_lock)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock_shared();
        co::sleep_for(std::chrono::seconds(1));
        mu.unlock_shared();
    });
    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(mu.try_lock_shared());
    mu.unlock_shared();
    c1.wait<void>();
}

TEST(co, co_shared_mutex_shared_lock_multi)
{
    co_shared_mutex mu;

    mu.lock_shared();
    mu.lock_shared();
    EXPECT_TRUE(mu.try_lock_shared());
    mu.unlock_shared();
    mu.unlock_shared();
    mu.unlock_shared();
}

TEST(co, co_shared_mutex_lock)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock();
        co::sleep_for(std::chrono::seconds(1));
        mu.unlock();
    });
    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock_shared());
    c1.wait<void>();
}

TEST(co, co_shared_mutex_lock2)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock_shared();
        co::sleep_for(std::chrono::seconds(1));
        mu.unlock_shared();
    });
    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock());
    c1.wait<void>();
}

TEST(co, co_shared_mutex_lock3)
{
    co_shared_mutex mu;

    co c1([&] {
        mu.lock();
        co::sleep_for(std::chrono::seconds(1));
        mu.unlock();
    });
    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock());
    c1.wait<void>();
}

TEST(co, co_shared_timed_mutex1)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock_shared();
        co::sleep_for(std::chrono::seconds(1));
        mu.unlock_shared();
    });
    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock_for(std::chrono::milliseconds(100)));
    EXPECT_TRUE(mu.try_lock_for(std::chrono::milliseconds(600)));
    mu.unlock();
    c1.wait<void>();
}

TEST(co, co_shared_timed_mutex2)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock();
        co::sleep_for(std::chrono::seconds(1));
        mu.unlock();
    });
    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock_for(std::chrono::milliseconds(100)));
    EXPECT_TRUE(mu.try_lock_for(std::chrono::milliseconds(600)));
    mu.unlock();
    c1.wait<void>();
}

TEST(co, co_shared_timed_mutex3)
{
    co_shared_timed_mutex mu;

    co c1([&] {
        mu.lock();
        co::sleep_for(std::chrono::seconds(1));
        mu.unlock();
    });
    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock_shared_for(std::chrono::milliseconds(100)));
    EXPECT_TRUE(mu.try_lock_shared_for(std::chrono::milliseconds(800)));
    mu.unlock_shared();
    c1.wait<void>();
}

TEST(co, co_condition_variable_notify_one)
{
    co_mutex              mu;
    co_condition_variable cond;
    int                   n = 100;

    co c1([&] {
        std::unique_lock<co_mutex> lck(mu);
        cond.wait(lck);
        n -= 5;
    });
    n /= 2;
    co::sleep_for(std::chrono::seconds(1));
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
        std::unique_lock<co_mutex> lck(mu);
        cond.wait(lck);
        n -= 5;
    });
    co c2([&] {
        std::unique_lock<co_mutex> lck(mu);
        cond.wait(lck);
        n -= 10;
    });
    n /= 2;
    co::sleep_for(std::chrono::seconds(1));
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
        co::sleep_for(std::chrono::seconds(1));
        n /= 2;
    });
    c1.detach();
    EXPECT_EQ(n, 100);
    std::unique_lock<co_mutex> lck(mu);
    cond.wait(lck);
    EXPECT_EQ(n, 50);
}

TEST(co, co_finished_event)
{
    int n = 100;
    co  c1([&] {
        co::sleep_for(std::chrono::milliseconds(100));
    });
    c1.detach();
    EXPECT_EQ(n, 100);
    c1.co_finished().register_callback([&]() {
        n /= 2;
    });
    co::sleep_for(std::chrono::milliseconds(500));
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
        EXPECT_FALSE(sem.try_acquire_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(50)));
    });
    sem.release(10);
    c1.wait<void>();
}
#include <chrono>
#include <gtest/gtest.h>
#include <mutex>
#define private public
#include "co.h"
#include "co_condition_variable.h"
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

class co_suite : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        co::init_co(co_manager::instance(co_o1_scheduler_factory::instance(),
                                         co_stack_factory::instance(),
                                         co_ctx_factory::instance(),
                                         co_env_factory::instance()));
    }

    static void TearDownTestCase()
    {
        co::uninit_co();
    }
};

TEST_F(co_suite, name)
{
    co c1({ with_name("test1") }, [this]() {
        EXPECT_EQ(co::this_co::name(), "test1");
    });
    c1.wait<void>();
}

TEST_F(co_suite, id)
{
    auto f = []() {
        for (int i = 0; i < 10; ++i)
        {
            printf("%s %llu %d\n", co::this_co::name().c_str(), co::this_co::id(), i);
            co::schedule_switch();
        }
    };
    co c1({ with_name("test1") }, f);
    co c2({ with_name("test1") }, f);
    c1.wait<void>();
    c2.wait<void>();
}

TEST_F(co_suite, my_thread)
{
    std::thread th([]() {
        printf("new thread: %u\n", gettid());
        co::convert_to_schedule_thread();
    });

    co c1([]() {
        printf("new co %llu in thread %u\n", co::this_co::id(), gettid());
    });

    c1.wait<void>();
    th.detach();
}

TEST_F(co_suite, detach)
{
    co c1([]() {
        for (int i = 0; i < 10000; ++i)
        {
            printf("count %d\n", i);
            co::schedule_switch();
        }
    });
    c1.wait<void>();
}

TEST_F(co_suite, ref)
{
    int t = 20;
    co  c1([](int& n) { n += 10; }, std::ref(t));
    c1.wait<void>();
    EXPECT_EQ(t, 30);
}

TEST_F(co_suite, return_value)
{
    co c1([](int n) { return n + 10; }, 25);
    EXPECT_EQ(c1.wait<int>(), 35);
}

TEST_F(co_suite, wait_timeout)
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
// TEST_F(co_suite, priority)
// {
//     std::vector<int> arr;
//     co               c1(
//         { with_priority(0) }, [](std::vector<int>& arr) {
//             co::sleep_for(std::chrono::milliseconds(50));
//             arr.push_back(100);
//             co::schedule_switch();
//             arr.push_back(200);
//             co::schedule_switch();
//             arr.push_back(300);
//             co::schedule_switch();
//         },
//         std::ref(arr));
//     co c2(
//         { with_priority(1) }, [](std::vector<int>& arr) {
//             co::sleep_for(std::chrono::milliseconds(50));
//             arr.push_back(400);
//             co::schedule_switch();
//             arr.push_back(500);
//             co::schedule_switch();
//             arr.push_back(600);
//             co::schedule_switch();
//         },
//         std::ref(arr));
//     c1.wait<void>();
//     c2.wait<void>();
//     std::vector<int> expect { 100, 200, 300, 400, 500, 600 };
//     EXPECT_EQ(arr, expect);
// }

TEST_F(co_suite, co_id)
{
    co_id id;
    co    c1([&id]() {
        id = co::this_co::id();
    });
    c1.wait<void>();
    EXPECT_EQ(c1.id(), id);
}

TEST_F(co_suite, co_id_name_after_detach)
{
    co_id id;
    co    c1([&id]() {
    });
    c1.wait<void>();
    c1.detach();
    EXPECT_EQ(c1.id(), 0);
    EXPECT_EQ(c1.name(), "");
}

TEST_F(co_suite, other_co_name)
{
    co c1({ with_name("zhangsan") }, []() {});
    c1.wait<void>();
    EXPECT_EQ(c1.name(), "zhangsan");
}

TEST_F(co_suite, co_spinlock_try_lock)
{
    co_spinlock mu;

    co c1([&]() {
        std::lock_guard<co_spinlock> lock(mu);
        co::sleep_for(std::chrono::seconds(1));
    });

    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock());
}

TEST_F(co_suite, co_spinlock_lock)
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

TEST_F(co_suite, co_mutex_try_lock)
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

TEST_F(co_suite, co_mutex_lock)
{
    co_mutex mu;

    int ret = 0;

    co c1([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            std::lock_guard<co_mutex> lock(mu);
            ret += i;
            co::schedule_switch();
        }
    });
    co c2([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            std::lock_guard<co_mutex> lock(mu);
            ret += i;
            co::schedule_switch();
        }
    });
    co c3([&]() {
        for (int i = 0; i < 1000; ++i)
        {
            std::lock_guard<co_mutex> lock(mu);
            ret -= i;
            co::schedule_switch();
        }
    });
    for (int i = 0; i < 1000; ++i)
    {
        std::lock_guard<co_mutex> lock(mu);
        ret -= i;
        co::schedule_switch();
    }

    c1.wait<void>();
    c2.wait<void>();
    c3.wait<void>();

    EXPECT_EQ(ret, 0);
}

TEST_F(co_suite, co_mutex_throw)
{
    co_mutex mu;
    EXPECT_THROW(mu.unlock(), co_error);
}

TEST_F(co_suite, co_recursive_mutex_lock)
{
    co_recursive_mutex mu;
    mu.lock();
    mu.lock();
    mu.lock();
    mu.unlock();
    mu.unlock();
    mu.unlock();
}

TEST_F(co_suite, co_recursive_mutex_trylock)
{
    co_recursive_mutex mu;
    EXPECT_TRUE(mu.try_lock());
    EXPECT_TRUE(mu.try_lock());
    EXPECT_TRUE(mu.try_lock());
    mu.unlock();
    mu.unlock();
    mu.unlock();
}

TEST_F(co_suite, co_timed_mutex)
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

TEST_F(co_suite, co_shared_mutex_shared_lock)
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

TEST_F(co_suite, co_shared_mutex_shared_lock_multi)
{
    co_shared_mutex mu;

    mu.lock_shared();
    mu.lock_shared();
    EXPECT_TRUE(mu.try_lock_shared());
    mu.unlock_shared();
    mu.unlock_shared();
    mu.unlock_shared();
}

TEST_F(co_suite, co_shared_mutex_lock)
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

TEST_F(co_suite, co_shared_mutex_lock2)
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

TEST_F(co_suite, co_shared_mutex_lock3)
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

TEST_F(co_suite, co_shared_timed_mutex1)
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

TEST_F(co_suite, co_shared_timed_mutex2)
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

TEST_F(co_suite, co_shared_timed_mutex3)
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

TEST_F(co_suite, co_condition_variable_notify_one)
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

TEST_F(co_suite, co_condition_variable_notify_all)
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

TEST_F(co_suite, co_condition_variable_notify_at_co_exit)
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

TEST_F(co_suite, co_finished_event)
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

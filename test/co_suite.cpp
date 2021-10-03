#include <gtest/gtest.h>
#define private public
#include "co.h"
#include "co_default_ctx_factory.h"
#include "co_default_env_factory.h"
#include "co_default_manager.h"
#include "co_default_stack_factory.h"
#include "co_o1_scheduler_factory.h"
#include "co_spinlock.h"

class co_suite : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        co::init_co(co_default_manager::instance(co_o1_scheduler_factory::instance(),
                                                 co_default_stack_factory::instance(),
                                                 co_default_ctx_factory::instance(),
                                                 co_default_env_factory::instance()));
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

TEST_F(co_suite, co_mutex_try_lock)
{
    co_spinlock mu;

    co c1([&]() {
        std::lock_guard<co_spinlock> lock(mu);
        co::sleep_for(std::chrono::seconds(1));
    });

    co::sleep_for(std::chrono::milliseconds(500));
    EXPECT_FALSE(mu.try_lock());
}

TEST_F(co_suite, co_mutex_lock)
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
#include <gtest/gtest.h>
#define private public
#include "co.h"
#include "co_default_manager.h"

class co_suite : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        co::init_co(co_default_manager::instance());
    }

    static void TearDownTestCase()
    {
        co::uninit_co();
    }
};

TEST_F(co_suite, name)
{
    co c1({ with_name("test1") }, [this]() {
        EXPECT_EQ(co::name(), "test1");
    });
    c1.wait<void>();
}

TEST_F(co_suite, id)
{
    auto f = []() {
        for (int i = 0; i < 10; ++i)
        {
            printf("%s %llu %d\n", co::name().c_str(), co::id(), i);
            co::schedule_switch();
        }
    };
    co c1({ with_name("test1") }, f);
    co c2({ with_name("test1") }, f);
    c1.wait<void>();
}

TEST_F(co_suite, my_thread)
{
    std::thread th([]() {
        printf("new thread: %llu\n", std::this_thread::get_id());
        co::convert_to_schedule_thread();
    });

    co c1([]() {
        printf("new co %llu in thread %llu\n", co::id(), std::this_thread::get_id());
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
    c1.detach();
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
    auto ret = c1.wait(std::chrono::milliseconds(100));
    EXPECT_FALSE(ret);
    ret = c1.wait(std::chrono::milliseconds(1000));
    EXPECT_TRUE(ret);
}

TEST_F(co_suite, priority)
{
    std::vector<int> arr;
    co               c1(
        { with_priority(0) }, [](std::vector<int>& arr) {
            arr.push_back(100);
            co::schedule_switch();
            arr.push_back(200);
            co::schedule_switch();
            arr.push_back(300);
            co::schedule_switch();
        },
        std::ref(arr));
    co c2(
        { with_priority(1) }, [](std::vector<int>& arr) {
            arr.push_back(400);
            co::schedule_switch();
            arr.push_back(500);
            co::schedule_switch();
            arr.push_back(600);
            co::schedule_switch();
        },
        std::ref(arr));
    std::vector<int> expect { 100, 200, 300, 400, 500, 600 };
    EXPECT_EQ(arr, expect);
}
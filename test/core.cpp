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
#include "cocpp/interface/co.h"
#include "cocpp/interface/co_pipeline.h"
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
using namespace std;
using namespace chrono_literals;

TEST(core, name)
{
    co c1({ with_name("test1") }, []() {
        EXPECT_EQ(this_co::name(), "test1");
    });
    c1.join();
}

TEST(core, id)
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
    c1.join();
    c2.join();
}

TEST(core, thread_convert)
{
    thread th([]() {
        printf("new thread: %u\n", gettid());
        co::schedule_in_this_thread();
    });

    co c1([]() {
        printf("new co %llu in thread %u\n", this_co::id(), gettid());
    });

    c1.join();
    th.detach();
}

TEST(core, detach)
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

TEST(core, ref)
{
    int t = 20;
    co  c1([](int& n) { n += 10; }, ref(t));
    c1.join();
    EXPECT_EQ(t, 30);
}

TEST(core, return_value)
{
    co c1([](int n) { return n + 10; }, 25);
    EXPECT_EQ(c1.wait<int>(), 35);
}

TEST(core, wait_timeout)
{
    co   c1([]() {
        this_co::sleep_for(1s);
      });
    auto ret = c1.wait_for(1ms);
    EXPECT_FALSE(ret);
    ret = c1.wait_for(10s);
    EXPECT_TRUE(ret);
}

TEST(core, this_co_id)
{
    co_id id;
    co    c1([&id]() {
        id = this_co::id();
       });
    c1.wait<void>();
    EXPECT_EQ(c1.id(), id);
}

TEST(core, get_id_name_after_detach)
{
    co c1([]() {
    });
    c1.join();
    c1.detach();
    EXPECT_EQ(c1.id(), 0ULL);
    EXPECT_EQ(c1.name(), "");
}

TEST(core, other_co_name)
{
    co c1({ with_name("zhangsan") }, []() {});
    c1.wait<void>();
    EXPECT_EQ(c1.name(), "zhangsan");
}

TEST(core, co_wait_priority)
{
    auto env = co::create_env(true);

    co c1({ with_priority(99), with_bind_env(env) }, [] {
        this_co::sleep_for(1s);
    });

    co c2({ with_priority(0), with_bind_env(env) }, [&] {
        c1.join();
    });

    c2.join();
}

TEST(core, co_shared_stack)
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

TEST(core, co_local)
{
    co c1([]() {
        CoLocal(name, string) = "hello";
        auto& value           = CoLocal(name, string);
        EXPECT_EQ(value, "hello");
    });

    c1.join();
    auto& value = CoLocal(name, string);
    EXPECT_EQ(value, "");
}

TEST(core, zone_edge)
{
    EXPECT_EQ(co_mem_pool::align_2_zone_edge__(1), 0ULL);
    EXPECT_EQ(co_mem_pool::align_2_zone_edge__(2), 1ULL);
    EXPECT_EQ(co_mem_pool::align_2_zone_edge__(3), 2ULL);
    EXPECT_EQ(co_mem_pool::align_2_zone_edge__(4), 2ULL);
}

TEST(core, exception)
{
    auto c = co([] { throw 1; });
    EXPECT_THROW(c.wait<int>(), int);
}

TEST(core, pipeline)
{
    int  source = 0;
    auto ch     = co_pipeline<int, 1>([&source]() -> std::optional<int> {
                  if (source < 10000)
                  {
                      return source++;
                  }
                  return std::nullopt;
              })
              | pipeline::left(2000)
              | pipeline::not_left(1000)
              | pipeline::fork(10, [](int n) -> int {
                    return n * 2;
                })
              | pipeline::filter([](int n) { return n % 3 == 0; })
              //   | pipeline::reduce([](int n, int m) { return n + m; }, 0)
              | pipeline::chan();

    for (auto&& p : ch)
    {
        cout << p << endl;
    }
    // EXPECT_EQ(ch.pop(), 90);
}
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

TEST(stl, vector)
{
    std::vector<std::string> v1;
    std::vector<std::string> v2;
    co                       c1([&] {
        for (auto i = 0; i < 10000; ++i)
        {
            v1.push_back(std::to_string(i));
            this_co::yield();
        }
    });
    co                       c2([&] {
        for (auto i = 0; i < 10000; ++i)
        {
            v2.push_back(std::to_string(i));
            this_co::yield();
        }
    });
    c1.wait<void>();
    c2.wait<void>();
    EXPECT_EQ(v1, v2);
}

TEST(any, co_any)
{
    co_any any(5);
    EXPECT_EQ(any.get<int>(), 5);
}
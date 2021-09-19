#include <gtest/gtest.h>

#define private public

#include "co_default_ctx_factory.h"
#include "co_default_env_factory.h"
#include "co_default_stack_factory.h"
#include "co_env.h"

class env_factory_suite : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        factory = co_default_env_factory::instance();
    }

    static void TearDownTestCase()
    {
    }

    static co_env_factory* factory;
};

co_env_factory* env_factory_suite::factory;

TEST_F(env_factory_suite, create_env)
{
    auto env = factory->create_env(1024);
    EXPECT_NE(env, nullptr);
    env->stop_schedule();
    factory->destroy_env(env);
}

TEST_F(env_factory_suite, create_env_from_this_thread)
{
    auto env = factory->create_env_from_this_thread(1024);
    EXPECT_NE(env, nullptr);
    factory->destroy_env(env);
}

TEST_F(env_factory_suite, default_factory)
{
    EXPECT_EQ(((co_default_env_factory*)factory)->stack_factory__, co_default_stack_factory::instance());
    EXPECT_EQ(((co_default_env_factory*)factory)->ctx_factory__, co_default_ctx_factory::instance());
}

TEST_F(env_factory_suite, set_factory)
{
    auto old_stack_factory = ((co_default_env_factory*)factory)->stack_factory__;
    auto old_ctx_factory   = ((co_default_env_factory*)factory)->ctx_factory__;
    factory->set_stack_factory(nullptr);
    factory->set_ctx_factory(nullptr);
    EXPECT_EQ(((co_default_env_factory*)factory)->stack_factory__, nullptr);
    EXPECT_EQ(((co_default_env_factory*)factory)->ctx_factory__, nullptr);
    factory->set_stack_factory(old_stack_factory);
    factory->set_ctx_factory(old_ctx_factory);
}

TEST_F(env_factory_suite, destory_nullptr)
{
    factory->destroy_env(nullptr);
}
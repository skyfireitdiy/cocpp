#include <gtest/gtest.h>

#define private public

#include "co_ctx.h"
#include "co_default_ctx_factory.h"

class ctx_factory_suite : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        factory = co_default_ctx_factory::instance();
    }

    static void TearDownTestCase()
    {
    }

    static co_ctx_factory* factory;
};

co_ctx_factory* ctx_factory_suite::factory;

TEST_F(ctx_factory_suite, create_ctx_default)
{
    co_ctx_config config;
    auto          ctx = factory->create_ctx(config);
    EXPECT_EQ("__unknown__", ctx->config().name);
    EXPECT_EQ(99, ctx->priority());
    EXPECT_EQ(CO_DEFAULT_STACK_SIZE, ctx->stack()->stack_size());
    factory->destroy_ctx(ctx);
}

TEST_F(ctx_factory_suite, create_ctx_with_name)
{
    auto          factory = co_default_ctx_factory::instance();
    co_ctx_config config;
    config.name = "test_name";
    auto ctx    = factory->create_ctx(config);
    EXPECT_EQ("test_name", ctx->config().name);
    factory->destroy_ctx(ctx);
}

TEST_F(ctx_factory_suite, create_ctx_with_normal_priority)
{
    auto          factory = co_default_ctx_factory::instance();
    co_ctx_config config;
    config.priority = 50;
    auto ctx        = factory->create_ctx(config);
    EXPECT_EQ(50, ctx->priority());
    factory->destroy_ctx(ctx);
}

TEST_F(ctx_factory_suite, create_ctx_with_negative_priority)
{
    auto          factory = co_default_ctx_factory::instance();
    co_ctx_config config;
    config.priority = -20;
    auto ctx        = factory->create_ctx(config);
    EXPECT_EQ(0, ctx->priority());
    factory->destroy_ctx(ctx);
}

TEST_F(ctx_factory_suite, create_ctx_with_max_priority)
{
    auto          factory = co_default_ctx_factory::instance();
    co_ctx_config config;
    config.priority = CO_MAX_PRIORITY;
    auto ctx        = factory->create_ctx(config);
    EXPECT_EQ(99, ctx->priority());
    factory->destroy_ctx(ctx);
}

TEST_F(ctx_factory_suite, create_ctx_with_huge_priority)
{
    auto          factory = co_default_ctx_factory::instance();
    co_ctx_config config;
    config.priority = CO_MAX_PRIORITY + 50;
    auto ctx        = factory->create_ctx(config);
    EXPECT_EQ(99, ctx->priority());
    factory->destroy_ctx(ctx);
}

TEST_F(ctx_factory_suite, set_stack_factory)
{
    auto old_stack_factory = ((co_default_ctx_factory*)factory)->stack_factory__;
    factory->set_stack_factory(nullptr);
    EXPECT_EQ(((co_default_ctx_factory*)factory)->stack_factory__, nullptr);
    factory->set_stack_factory(old_stack_factory);
}

TEST_F(ctx_factory_suite, destory_nullptr)
{
    factory->destroy_ctx(nullptr);
}
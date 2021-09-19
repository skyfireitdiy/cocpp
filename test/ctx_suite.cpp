#include "co_default_entry.h"
#include <any>
#include <gtest/gtest.h>
#define private public

#include "co_default_ctx.h"
#include "co_default_ctx_factory.h"
#include "co_default_env_factory.h"

class ctx_suite : public testing::Test
{
public:
    void SetUp() override
    {
        ctx = factory->create_ctx(co_ctx_config {});
    }

    void TearDown() override
    {
        factory->destroy_ctx(ctx);
    }

    static void SetUpTestCase()
    {
        factory = co_default_ctx_factory::instance();
    }

    static void TearDownTestCase()
    {
    }

    static co_ctx_factory* factory;
    co_ctx*                ctx;
};

co_ctx_factory* ctx_suite::factory;

TEST_F(ctx_suite, get_stack)
{
    EXPECT_NE(ctx->stack(), nullptr);
}

TEST_F(ctx_suite, get_state)
{
    EXPECT_EQ(ctx->state(), co_state::suspended);
}

TEST_F(ctx_suite, get_regs)
{
    EXPECT_NE(ctx->regs(), nullptr);
}

TEST_F(ctx_suite, set_state)
{
    ctx->set_state(co_state::finished);
    EXPECT_EQ(ctx->state(), co_state::finished);
}

bool operator==(const co_ctx_config& a, const co_ctx_config& b)
{
    return a.name == b.name && a.priority == b.priority && a.stack_size == b.stack_size && a.startup == b.startup;
}

TEST_F(ctx_suite, get_config)
{
    EXPECT_EQ(ctx->config(), co_ctx_config {});
}

TEST_F(ctx_suite, get_ret_ref)
{
    auto& ret = ctx->ret_ref();
    ret       = 5;
    EXPECT_EQ(std::any_cast<int>(ctx->ret_ref()), 5);
}

TEST_F(ctx_suite, get_default_env)
{
    EXPECT_EQ(ctx->env(), nullptr);
}

TEST_F(ctx_suite, set_env)
{
    auto env = co_default_env_factory::instance()->create_env_from_this_thread(1024);
    ctx->set_env(env);
    EXPECT_EQ(ctx->env(), env);
    co_default_env_factory::instance()->destroy_env(env);
}

TEST_F(ctx_suite, test_default_flag)
{
    EXPECT_FALSE(ctx->test_flag(CO_CTX_FLAG_WAITING));
    EXPECT_FALSE(ctx->test_flag(CO_CTX_FLAG_HANDLE_BY_CO));
}

TEST_F(ctx_suite, set_flag)
{
    ctx->set_flag(CO_CTX_FLAG_WAITING);
    EXPECT_TRUE(ctx->test_flag(CO_CTX_FLAG_WAITING));
    EXPECT_FALSE(ctx->test_flag(CO_CTX_FLAG_HANDLE_BY_CO));
    ctx->set_flag(CO_CTX_FLAG_HANDLE_BY_CO);
    EXPECT_TRUE(ctx->test_flag(CO_CTX_FLAG_WAITING));
    EXPECT_TRUE(ctx->test_flag(CO_CTX_FLAG_HANDLE_BY_CO));
    ctx->reset_flag(CO_CTX_FLAG_WAITING);
    EXPECT_FALSE(ctx->test_flag(CO_CTX_FLAG_WAITING));
    EXPECT_TRUE(ctx->test_flag(CO_CTX_FLAG_HANDLE_BY_CO));
    ctx->reset_flag(CO_CTX_FLAG_HANDLE_BY_CO);
    EXPECT_FALSE(ctx->test_flag(CO_CTX_FLAG_WAITING));
    EXPECT_FALSE(ctx->test_flag(CO_CTX_FLAG_HANDLE_BY_CO));
}

TEST_F(ctx_suite, get_default_priority)
{
    EXPECT_EQ(ctx->priority(), CO_MAX_PRIORITY - 1);
}

TEST_F(ctx_suite, set_normal_priority)
{
    ctx->set_priority(55);
    EXPECT_EQ(ctx->priority(), 55);
}

TEST_F(ctx_suite, set_max_priority)
{
    ctx->set_priority(CO_MAX_PRIORITY);
    EXPECT_EQ(ctx->priority(), 99);
}

TEST_F(ctx_suite, set_negative_priority)
{
    ctx->set_priority(-50);
    EXPECT_EQ(ctx->priority(), 0);
}

#ifdef __GNUC__
#ifdef __x86_64__
TEST_F(ctx_suite, init_regs)
{
    EXPECT_EQ(((co_default_ctx*)ctx)->regs__[co_default_ctx::reg_index_RSP__], ctx->stack()->stack_top() - sizeof(void*));
    EXPECT_EQ(((co_default_ctx*)ctx)->regs__[co_default_ctx::reg_index_RSP__], ((co_default_ctx*)ctx)->regs__[co_default_ctx::reg_index_RBP__]);
    EXPECT_EQ(((co_default_ctx*)ctx)->regs__[co_default_ctx::reg_index_RDI__], (co_byte*)ctx);
    EXPECT_EQ(((co_default_ctx*)ctx)->regs__[co_default_ctx::reg_index_RIP__], (co_byte*)co_default_entry);
}
#endif
#endif
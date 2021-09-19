#include "co_default_ctx_factory.h"
#include "co_default_ctx.h"
#include "co_stack_factory.h"

#include <cassert>

co_ctx* co_default_ctx_factory ::create_ctx(const co_ctx_config& config)
{
    auto ret = new co_default_ctx(stack_factory__->create_stack(config.stack_size), config);
    assert(ret != nullptr);
    // CO_DEBUG("create ctx: %s %p", ret->config().name.c_str(), ret);
    return ret;
}

void co_default_ctx_factory::destroy_ctx(co_ctx* ctx)
{
    if (ctx == nullptr)
    {
        CO_ERROR("destory nullptr ctx");
        return;
    }
    // CO_DEBUG("destory ctx: %s %p", ctx->config().name.c_str(), ctx);
    auto stack = ctx->stack();
    delete ctx;
    stack_factory__->destroy_stack(stack);
}

void co_default_ctx_factory::set_stack_factory(co_stack_factory* stack_factory)
{
    stack_factory__ = stack_factory;
}
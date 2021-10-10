#include "co_ctx_factory.h"
#include "co_ctx.h"
#include "co_manager.h"
#include "co_stack_factory.h"

#include <cassert>

co_ctx* co_ctx_factory ::create_ctx(const co_ctx_config& config)
{
    auto ret = new co_ctx(manager__->stack_factory()->create_stack(config.stack_size), config);
    assert(ret != nullptr);
    // CO_O_DEBUG("create ctx: %s %p", ret->config().name.c_str(), ret);
    return ret;
}

void co_ctx_factory::destroy_ctx(co_ctx* ctx)
{
    assert(ctx != nullptr);
    // CO_O_DEBUG("destory ctx: %s %p", ctx->config().name.c_str(), ctx);
    auto stack = ctx->stack();
    delete ctx;
    manager__->stack_factory()->destroy_stack(stack);
}

void co_ctx_factory::set_manager(co_manager* manager)
{
    manager__ = manager;
}
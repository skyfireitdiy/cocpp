#include "co_ctx_factory.h"
#include "co_ctx.h"
#include "co_entry.h"
#include "co_manager.h"
#include "co_stack_factory.h"

#include <cassert>

CO_NAMESPACE_BEGIN

co_ctx* co_ctx_factory ::create_ctx(const co_ctx_config& config)
{
    auto ret = ctx_pool__.create_obj(config.shared_stack ? nullptr : co_stack_factory::instance()->create_stack(config.stack_size), config);
    assert(ret != nullptr);
    if (config.bind_env != nullptr)
    {
        ret->set_flag(CO_CTX_FLAG_BIND);
    }
    if (config.shared_stack)
    {
        ret->set_flag(CO_CTX_FLAG_SHARED_STACK);
    }
    // CO_O_DEBUG("create ctx: %s %p", ret->config().name.c_str(), ret);
    return ret;
}

void co_ctx_factory::destroy_ctx(co_ctx* ctx)
{
    assert(ctx != nullptr);
    // CO_O_DEBUG("destory ctx: %s %p", ctx->config().name.c_str(), ctx);
    auto stack = ctx->stack();
    ctx_pool__.destroy_obj(ctx);
    co_stack_factory::instance()->destroy_stack(stack);
}

void co_ctx_factory::free_obj_pool()
{
    ctx_pool__.clear_free_object();
}

CO_NAMESPACE_END
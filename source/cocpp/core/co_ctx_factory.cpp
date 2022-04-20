#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_stack_factory.h"
#include "cocpp/utils/co_any.h"

#include <cassert>

using namespace std;

CO_NAMESPACE_BEGIN

co_ctx* co_ctx_factory ::create_ctx(const co_ctx_config& config, function<void(co_any&)> entry)
{
    auto ret = ctx_pool__.create_obj(config.shared_stack ? nullptr : co_stack_factory::instance()->create_stack(config.stack_size), config, entry);
    assert(ret != nullptr);
    if (config.bind_env != nullptr)
    {
        ret->set_flag(CO_CTX_FLAG_BIND);
    }
    if (config.shared_stack)
    {
        ret->set_flag(CO_CTX_FLAG_SHARED_STACK);
    }
    return ret;
}

void co_ctx_factory::destroy_ctx(co_ctx* ctx)
{
    CoPreemptGuard();
    assert(ctx != nullptr);
    auto stack = ctx->stack();
    ctx_pool__.destroy_obj(ctx);
    co_stack_factory::instance()->destroy_stack(stack);
}

void co_ctx_factory::free_obj_pool()
{
    ctx_pool__.clear_free_object();
}

CO_NAMESPACE_END
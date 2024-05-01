#include "cocpp/core/co_ctx_factory.h"
#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_stack_factory.h"
#include "cocpp/utils/co_any.h"

#include <cassert>

using namespace std;

CO_NAMESPACE_BEGIN

co_ctx *co_ctx_factory ::create_ctx(const co_ctx_config &config, function<void(co_any &)> entry)
{
    auto ret = new co_ctx(config.shared_stack ? nullptr : co_stack_factory::create_stack(config.stack_size), config, entry);
    if (ret == nullptr)
    {
        CO_ERROR("create ctx failed, stack size: %ld", config.stack_size);
        throw std::bad_alloc();
    }
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

void co_ctx_factory::destroy_ctx(co_ctx *ctx)
{
    CoPreemptGuard();
    assert(ctx != nullptr);
    auto shared_stack = ctx->test_flag(CO_CTX_FLAG_SHARED_STACK);
    auto stack = ctx->stack();
    delete ctx;
    if (!shared_stack)
    {
        co_stack_factory::destroy_stack(stack);
    }
}

CO_NAMESPACE_END

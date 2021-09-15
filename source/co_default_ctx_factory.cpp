#include "co_default_ctx_factory.h"
#include "co_default_ctx.h"
#include "co_stack_factory.h"

co_ctx* co_default_ctx_factory ::create_ctx(const co_ctx_config& config)
{
    return new co_default_ctx(stack_factory__->create_stack(config.stack_size), config);
}

void co_default_ctx_factory::destroy_ctx(co_ctx* ctx)
{
    stack_factory__->destroy_stack(ctx->stack());
    delete ctx;
}

void co_default_ctx_factory::set_stack_factory(co_stack_factory* stack_factory)
{
    stack_factory__ = stack_factory;
}
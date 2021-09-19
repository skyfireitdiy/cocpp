#include "co_default_env_factory.h"
#include "co_default_env.h"

#include <cassert>

co_env* co_default_env_factory::create_env(size_t stack_size)
{
    auto idle_ctx = create_idle_ctx__();
    auto ret      = new co_default_env(scheduler_factory__->create_scheduler(), idle_ctx, stack_factory__->create_stack(stack_size), true);
    assert(ret != nullptr);
    idle_ctx->set_state(co_state::running);
    // CO_DEBUG("create env: %p", ret);
    return ret;
}

void co_default_env_factory::destroy_env(co_env* env)
{
    if (env == nullptr)
    {
        CO_ERROR("destory nullptr env");
        return;
    }
    // 此处需要先删除env对象，然后再销毁内部资源，因为在env销毁前，内部资源可能还正在被使用
    // CO_DEBUG("destroy env: %p", env);
    auto idle_ctx  = env->idle_ctx();
    auto scheduler = env->scheduler();
    auto stack     = env->shared_stack();
    delete env;
    ctx_factory__->destroy_ctx(idle_ctx);
    scheduler_factory__->destroy_scheduler(scheduler);
    stack_factory__->destroy_stack(stack);
}

void co_default_env_factory::set_stack_factory(co_stack_factory* stack_factory)
{
    stack_factory__ = stack_factory;
}

void co_default_env_factory::set_ctx_factory(co_ctx_factory* ctx_factory)
{
    ctx_factory__ = ctx_factory;
}

co_env* co_default_env_factory::create_env_from_this_thread(size_t stack_size)
{
    auto idle_ctx = create_idle_ctx__();
    auto ret      = new co_default_env(scheduler_factory__->create_scheduler(), idle_ctx, stack_factory__->create_stack(stack_size), false);
    idle_ctx->set_state(co_state::running);
    // CO_DEBUG("create env: %p", ret);
    return ret;
}

co_ctx* co_default_env_factory::create_idle_ctx__()
{
    return ctx_factory__->create_ctx(co_ctx_config { nullptr, 0, nullptr, "idle", CO_IDLE_CTX_PRIORITY });
}
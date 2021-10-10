#include "co_env_factory.h"
#include "co_ctx.h"
#include "co_env.h"
#include "co_manager.h"
#include <cassert>

co_env* co_env_factory::create_env(size_t stack_size)
{
    auto idle_ctx = create_idle_ctx__();
    auto ret      = new co_env(manager__->scheduler_factory()->create_scheduler(), idle_ctx, true);
    assert(ret != nullptr);
    idle_ctx->set_state(co_state::running);
    // CO_O_DEBUG("create env: %p", ret);
    return ret;
}

void co_env_factory::destroy_env(co_env* env)
{
    assert(env != nullptr);
    // 此处需要先删除env对象，然后再销毁内部资源，因为在env销毁前，内部资源可能还正在被使用
    // CO_O_DEBUG("destroy env: %p", env);
    auto idle_ctx  = env->idle_ctx();
    auto scheduler = env->scheduler();
    delete env;
    manager__->ctx_factory()->destroy_ctx(idle_ctx);
    manager__->scheduler_factory()->destroy_scheduler(scheduler);
}

void co_env_factory::set_manager(co_manager* manager)
{
    manager__ = manager;
}

co_env* co_env_factory::create_env_from_this_thread(size_t stack_size)
{
    auto idle_ctx = create_idle_ctx__();
    auto ret      = new co_env(manager__->scheduler_factory()->create_scheduler(), idle_ctx, false);
    idle_ctx->set_state(co_state::running);
    // CO_O_DEBUG("create env: %p", ret);
    return ret;
}

co_ctx* co_env_factory::create_idle_ctx__()
{
    co_ctx_config config;
    config.name       = "idle";
    config.stack_size = 0;
    config.priority   = CO_IDLE_CTX_PRIORITY;
    return manager__->ctx_factory()->create_ctx(config);
}
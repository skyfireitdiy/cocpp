#include "cocpp/core/co_env_factory.h"
#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include <cassert>
#include <utility>

CO_NAMESPACE_BEGIN

co_env* co_env_factory::create_env(size_t stack_size)
{
    auto idle_ctx = create_idle_ctx__();
    auto ret      = env_pool__.create_obj(stack_size, idle_ctx, true);
    assert(ret != nullptr);
    idle_ctx->set_state(co_state::running);
    return ret;
}

void co_env_factory::free_obj_pool()
{
    env_pool__.clear_free_object();
}

void co_env_factory::destroy_env(co_env* env)
{
    assert(env != nullptr);
    // 此处需要先删除env对象，然后再销毁内部资源，因为在env销毁前，内部资源可能还正在被使用
    auto idle_ctx     = env->idle_ctx__;
    auto shared_stack = env->shared_stack__;
    env_pool__.destroy_obj(env);
    co_ctx_factory::instance()->destroy_ctx(idle_ctx);
    if (shared_stack != nullptr)
    {
        stack_factory__->destroy_stack(shared_stack);
    }
}

co_env* co_env_factory::create_env_from_this_thread(size_t stack_size)
{
    auto idle_ctx = create_idle_ctx__();
    auto ret      = env_pool__.create_obj(stack_size, idle_ctx, false);
    idle_ctx->set_state(co_state::running);
    return ret;
}

co_ctx* co_env_factory::create_idle_ctx__()
{
    co_ctx_config config;
    config.name       = "idle";
    config.stack_size = 0;
    config.priority   = CO_IDLE_CTX_PRIORITY;
    auto ret          = co_ctx_factory::instance()->create_ctx(config, nullptr);
    ret->set_flag(CO_CTX_FLAG_IDLE);
    return ret;
}

CO_NAMESPACE_END
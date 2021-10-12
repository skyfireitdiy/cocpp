#include "co_ctx.h"
#include "co_define.h"
#include "co_env.h"
#include "co_scheduler.h"
#include <cassert>

CO_NAMESPACE_BEGIN

co_stack* co_ctx::stack() const
{
    return stack__;
}

co_state co_ctx::state() const
{
    return state__;
}

void co_ctx::set_state(co_state state)
{
    // finished 状态的ctx不再更新
    if (state__ != co_state::finished)
    {
        state__ = state;
    }
}

co_byte** co_ctx::regs()
{
    return reinterpret_cast<co_byte**>(&regs__);
}

const co_ctx_config& co_ctx::config() const
{
    return config__;
}

std::any& co_ctx::ret_ref()
{
    return ret__;
}

void co_ctx::set_env(co_env* env)
{
    env__ = env;
}

co_env* co_ctx::env() const
{
    return env__;
}

co_ctx::co_ctx(co_stack* stack, const co_ctx_config& config)
    : stack__(stack)
    , state__(co_state::created)
    , config__(config)
{
    set_priority(config.priority);
}

int co_ctx::priority() const
{
    return priority__;
}

void co_ctx::set_priority(int priority)
{
    int old_priority = priority__;
    if (priority >= CO_MAX_PRIORITY)
    {
        priority = CO_MAX_PRIORITY - 1;
    }
    if (priority < 0)
    {
        priority = 0;
    }
    priority__ = priority;
    if (env__ == nullptr) // 首次调用的时候env为空
    {
        return;
    }
    if (old_priority != priority__ && !test_flag(CO_CTX_FLAG_IDLE))
    {
        env__->scheduler()->change_priority(old_priority, priority__, this);
    }
}

bool co_ctx::can_destroy() const
{
    return !test_flag(CO_CTX_FLAG_LOCKED);
}

void co_ctx::lock_destroy()
{
    set_flag(CO_CTX_FLAG_LOCKED);
}

void co_ctx::unlock_destroy()
{
    reset_flag(CO_CTX_FLAG_LOCKED);
}

CO_NAMESPACE_END
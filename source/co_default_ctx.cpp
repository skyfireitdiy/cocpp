#include "co_default_ctx.h"
#include "co_define.h"
#include <cassert>

co_stack* co_default_ctx::stack() const
{
    return stack__;
}

co_state co_default_ctx::state() const
{
    return state__;
}

void co_default_ctx::set_state(co_state state)
{
    // finished 状态的ctx不再更新
    if (state__ != co_state::finished)
    {
        state__ = state;
    }
}

co_byte** co_default_ctx::regs()
{
    return reinterpret_cast<co_byte**>(&regs__);
}

const co_ctx_config& co_default_ctx::config() const
{
    return config__;
}

std::any& co_default_ctx::ret_ref()
{
    return ret__;
}

void co_default_ctx::set_env(co_env* env)
{
    env__ = env;
}

co_env* co_default_ctx::env() const
{
    return env__;
}

co_default_ctx::co_default_ctx(co_stack* stack, const co_ctx_config& config)
    : stack__(stack)
    , state__(co_state::created)
    , config__(config)
{
    set_priority(config.priority);
}

int co_default_ctx::priority() const
{
    return priority__;
}

void co_default_ctx::set_priority(int priority)
{
    if (priority >= CO_MAX_PRIORITY)
    {
        priority = CO_MAX_PRIORITY - 1;
    }
    if (priority < 0)
    {
        priority = 0;
    }
    priority__ = priority;
}

bool co_default_ctx::can_destroy() const
{
    return !test_flag(CO_CTX_FLAG_LOCKED);
}

void co_default_ctx::lock_destroy()
{
    set_flag(CO_CTX_FLAG_LOCKED);
}

void co_default_ctx::unlock_destroy()
{
    reset_flag(CO_CTX_FLAG_LOCKED);
}

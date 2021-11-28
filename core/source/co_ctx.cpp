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
    std::lock_guard<std::mutex> lock(state__.lock);
    return state__.state;
}

void co_ctx::set_state(co_state state)
{
    std::lock_guard<std::mutex> lock(state__.lock);
    // finished 状态的ctx不再更新
    if (state__.state != co_state::finished)
    {
        co_state old_state = state__.state;
        state__.state      = state;
        state_changed().pub(old_state, state);
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
    env_set().pub(env__);
}

co_env* co_ctx::env() const
{
    return env__;
}

co_ctx::co_ctx(co_stack* stack, const co_ctx_config& config)
    : stack__(stack)
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
        priority_changed().pub(old_priority, priority__.load());
    }
}

bool co_ctx::can_schedule() const
{
    return state() != co_state::finished;
}

bool co_ctx::can_destroy() const
{
    return !test_flag(CO_CTX_FLAG_LOCKED);
}

void co_ctx::lock_destroy()
{
    set_flag(CO_CTX_FLAG_LOCKED);
    locked_destroy().pub();
}

void co_ctx::unlock_destroy()
{
    reset_flag(CO_CTX_FLAG_LOCKED);
    unlocked_destroy().pub();
}

void co_ctx::set_stack(co_stack* stack)
{
    // CO_O_DEBUG("set stack: %p", stack);
    stack__ = stack;
    stack_set().pub(stack__);
}

bool co_ctx::can_move() const
{
    return !(state() == co_state::running || test_flag(CO_CTX_FLAG_BIND) || test_flag(CO_CTX_FLAG_SHARED_STACK) || test_flag(CO_CTX_FLAG_SWITCHING));
}

std::string co_ctx::name() const
{
    return config().name;
}

co_id co_ctx::id() const
{
    return reinterpret_cast<co_id>(this);
}

CO_NAMESPACE_END
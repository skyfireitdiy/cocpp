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
        priority_changed().pub(old_priority, priority__);
    }
}

bool co_ctx::can_schedule() const
{
    return state() != co_state::finished && !test_flag(CO_CTX_FLAG_WAITING);
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

void co_ctx::enter_wait_rc_state(int rc_type, void* rc)
{
    std::lock_guard<co_spinlock> lock(wait_data__.mu);
    wait_data__.type = rc_type;
    wait_data__.rc   = rc;
    set_flag(CO_CTX_FLAG_WAITING);
}

void co_ctx::leave_wait_rc_state()
{
    reset_flag(CO_CTX_FLAG_WAITING);
    env__->ctx_leave_wait_state(this);
    env__->wake_up();
}

CO_NAMESPACE_END
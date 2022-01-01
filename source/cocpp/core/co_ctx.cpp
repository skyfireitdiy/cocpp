#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/utils/co_any.h"
#include "cocpp/utils/co_state_manager.h"

#include <cassert>
#include <mutex>

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

co_any& co_ctx::ret_ref()
{
    return ret__;
}

void co_ctx::set_env(co_env* env)
{
    std::lock_guard<co_spinlock> lock(env_lock__);
    env__ = env;
    env_set().pub(env__);
}

co_env* co_ctx::env() const
{
    std::lock_guard<co_spinlock> lock(env_lock__);
    return env__;
}

co_ctx::co_ctx(co_stack* stack, const co_ctx_config& config, std::function<void(co_any&)> entry)
    : stack__(stack)
    , config__(config)
    , entry__(entry)
{
    set_priority(config.priority);
}

std::function<void(co_any&)> co_ctx::entry() const
{
    return entry__;
}

void co_ctx::real_entry(co_ctx* ctx)
{
    ctx->entry()(ctx->ret_ref());
    // CO_DEBUG("ctx %s %p finished", ctx->config().name.c_str(), ctx);
    ctx->set_state(co_state::finished);
    ctx->finished().pub();
    assert(ctx->env() != nullptr);
    ctx->env()->schedule_switch(); // 此处的ctx对应的env不可能为空，如果为空，这个ctx就不可能被调度
}

size_t co_ctx::priority() const
{
    std::scoped_lock lock(priority_lock__);
    return priority__;
}

void co_ctx::set_priority(int priority)
{
    if (test_flag(CO_CTX_FLAG_IDLE))
    {
        return;
    }
    std::unique_lock<co_spinlock> lock(priority_lock__);
    int                           old_priority = priority__;
    if (priority >= CO_MAX_PRIORITY)
    {
        priority = CO_MAX_PRIORITY - 1;
    }
    if (priority < 0)
    {
        priority = 0;
    }
    priority__ = priority;
    {
        std::lock_guard<co_spinlock> lock(env_lock__);
        if (env__ == nullptr) // 首次调用的时候env为空
        {
            return;
        }
    }
    if (old_priority != priority__)
    {
        lock.unlock(); // 在priority_changed 事件处理中，可能会修改priority__，所以需要解锁
        priority_changed().pub(old_priority, priority__);
        lock.lock();
    }
}

void co_ctx::set_state(const co_state& state)
{
    // TODO 此处可以优化
    std::scoped_lock lock(env_lock__);
    if (env__ != nullptr)
    {
        std::scoped_lock lock(env__->sleep_lock());
        state_manager__.set_state(state);
        state_set().pub(state);
        return;
    }
    state_manager__.set_state(state);
    state_set().pub(state);
}

void co_ctx::set_flag(size_t flag)
{
    // TODO 此处可以优化
    std::scoped_lock lock(env_lock__);
    if (env__ != nullptr)
    {
        std::scoped_lock lock(env__->sleep_lock());
        flag_manager__.set_flag(flag);
        flag_set().pub(flag);
        return;
    }
    flag_manager__.set_flag(flag);
    flag_set().pub(flag);
}

void co_ctx::reset_flag(size_t flag)
{
    // TODO 此处可以优化
    std::scoped_lock lock(env_lock__);
    if (env__ != nullptr)
    {
        std::scoped_lock lock(env__->sleep_lock());
        flag_manager__.reset_flag(flag);
        flag_reset().pub(flag);
        return;
    }
    flag_manager__.reset_flag(flag);
    flag_reset().pub(flag);
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
    stack__ = stack;
    stack_set().pub(stack__);
}

bool co_ctx::can_move() const
{
    return !(state() == co_state::running || state() == co_state::finished || test_flag(CO_CTX_FLAG_BIND) || test_flag(CO_CTX_FLAG_SHARED_STACK) || test_flag(CO_CTX_FLAG_SWITCHING));
}

std::string co_ctx::name() const
{
    return config().name;
}

co_id co_ctx::id() const
{
    return reinterpret_cast<co_id>(this);
}

void co_ctx::enter_wait_resource_state(int rc_type, void* rc)
{
    std::lock_guard<co_spinlock> lock(wait_data__.mu);
    wait_data__.type     = rc_type;
    wait_data__.resource = rc;
    set_flag(CO_CTX_FLAG_WAITING);
    std::lock_guard<co_spinlock> env_lock(env_lock__);
    env__->ctx_enter_wait_state(this);
    wait_resource_state_entered().pub(rc_type, rc);
}

void co_ctx::leave_wait_resource_state()
{
    reset_flag(CO_CTX_FLAG_WAITING);
    std::lock_guard<co_spinlock> lock(env_lock__);
    env__->ctx_leave_wait_state(this);
    env__->wake_up();
    wait_resource_state_leaved().pub();
}

CO_NAMESPACE_END
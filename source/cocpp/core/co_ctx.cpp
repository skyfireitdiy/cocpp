#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/core/co_stack.h"
#include "cocpp/core/co_type.h"
#include "cocpp/core/co_vos.h"
#include "cocpp/exception/co_exception.h"
#include "cocpp/utils/co_any.h"
#include "cocpp/utils/co_state_manager.h"

#include <algorithm>
#include <cassert>
#include <exception>
#include <mutex>
#include <sstream>

using namespace std;

CO_NAMESPACE_BEGIN

co_stack *co_ctx::stack() const
{
    return stack__;
}

co_byte **co_ctx::regs()
{
    return reinterpret_cast<co_byte **>(&regs__);
}

const co_ctx_config &co_ctx::config() const
{
    return config__;
}

co_any &co_ctx::ret_ref()
{
    return ret__;
}

void co_ctx::set_env(co_env *env)
{
    env__ = env;
}

co_env *co_ctx::env() const
{
    return env__;
}

co_ctx::co_ctx(co_stack *stack, const co_ctx_config &config, function<void(co_any &)> entry)
    : stack__(stack)
    , config__(config)
    , entry__(entry)
{
    set_priority(config.priority);
}

function<void(co_any &)> co_ctx::entry() const
{
    return entry__;
}

void co_ctx::real_entry(co_ctx *ctx)
{
    try
    {
        ctx->entry()(ctx->ret_ref());
    }
    catch (...)
    {
        ctx->exception__ = current_exception();
    }
    CoPreemptGuard();
    // CO_DEBUG("ctx %s %p finished", ctx->config().name.c_str(), ctx);
    ctx->lock_finished_state();
    ctx->set_state(co_state::finished);
    ctx->unlock_finished_state();
    ctx->finished().pub();
    assert(ctx->env() != nullptr);
    ctx->env()->schedule_switch(); // 此处的ctx对应的env不可能为空，如果为空，这个ctx就不可能被调度
}

size_t co_ctx::priority() const
{
    scoped_lock lock(priority_lock__);
    return priority__;
}

void co_ctx::set_priority(int priority)
{
    if (test_flag(CO_CTX_FLAG_IDLE))
    {
        return;
    }
    unique_lock lock(priority_lock__);
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
    {
        if (env__ == nullptr) // 首次调用的时候env为空
        {
            return;
        }
    }
    if (old_priority != priority__)
    {
        // NOTE: maybe change priority in event handler
        priority_changed().pub(old_priority, priority__);
    }
}

void co_ctx::set_flag(size_t flag)
{
    flag_manager__.set_flag(flag);
}

void co_ctx::unset_flag(size_t flag)
{
    flag_manager__.unset_flag(flag);
}

void co_ctx::check_and_rethrow_exception()
{
    if (exception__)
    {
        auto e = exception__;
        exception__ = nullptr; // 仅第一次抛出异常
        rethrow_exception(e);
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
}

void co_ctx::unlock_destroy()
{
    unset_flag(CO_CTX_FLAG_LOCKED);
}

void co_ctx::set_stack(co_stack *stack)
{
    stack__ = stack;
}

bool co_ctx::can_move() const
{
    return !(state() == co_state::running || state() == co_state::finished || test_flag(CO_CTX_FLAG_BIND) || test_flag(CO_CTX_FLAG_SHARED_STACK));
}

string co_ctx::name() const
{
    return config().name;
}

co_id co_ctx::id() const
{
    return reinterpret_cast<co_id>(this);
}

void co_ctx::enter_wait_resource_state(void *rc)
{
    {
        scoped_lock lock(wait_data__.mu);
        wait_data__.resource.insert(rc);
        set_flag(CO_CTX_FLAG_WAITING);
    }

    env__->ctx_enter_wait_state(this);
}

void co_ctx::leave_wait_resource_state(void *rc)
{
    {
        scoped_lock lock(wait_data__.mu);
        wait_data__.resource.erase(rc);
        if (wait_data__.resource.empty())
        {
            unset_flag(CO_CTX_FLAG_WAITING);
        }
    }
    if (!test_flag(CO_CTX_FLAG_WAITING))
    {
        env__->ctx_leave_wait_state(this);
        env__->wake_up();
    }
}

void co_ctx::lock_finished_state()
{
    finish_lock__.lock();
}

void co_ctx::unlock_finished_state()
{
    finish_lock__.unlock();
}

string co_ctx::ctx_info() const
{
    stringstream ss;
    ss << "------------ ctx start -----------" << endl;
    // config
    ss << "name:" << config__.name << endl;
    ss << "stack size:" << config__.stack_size << endl;
    ss << "config priority:" << config__.priority << endl;
    ss << "shared stack:" << (config__.shared_stack ? "true" : "false") << endl;
    ss << "bind env:" << (config__.bind_env ? "true" : "false") << endl;

    // ret
    ss << "ret: 0x" << hex << &ret__ << endl
       << dec;

    // priority
    ss << "realtime priority:" << priority__ << endl;

    // env
    ss << "env: 0x" << hex << env__ << endl
       << dec;

    // locals
    ss << "locals: \n";
    for (auto &p : locals__)
    {
        ss << "  " << p.first << ": 0x" << hex << p.second << dec << endl;
    }

    // exception
    ss << (exception__ ? "exception: true" : "exception: false") << endl;

    // regs
    ss << "regs: \n";
    auto context = reinterpret_cast<const sigcontext_64 *>(&regs__);
    ss << regs_info(context) << endl;

    // stack
    if (stack__)
    {
        ss << "stack:" << endl;
        ss << stack__->stack_info() << endl;
        if ((void *)context->sp < (void *)stack__->stack_top() && (void *)context->sp >= (void *)stack__->stack())
        {
            ss << dump_memory((co_byte *)context->sp, stack__->stack_top() - (co_byte *)context->sp) << endl;
        }
        else
        {
            ss << "stack ok" << endl;
        }
    }
    else
    {
        ss << "stack:null" << endl;
    }

    // flags
    ss << "flags: \n";
    for (auto i = 0; i < CO_CTX_FLAG_MAX; ++i)
    {
        ss << i << ": " << (test_flag(i) ? "true" : "false") << endl;
    }

    // state
    ss << "state: " << static_cast<int>(state()) << endl;

    ss << "------------ ctx end -----------" << endl;

    return ss.str();
}

void co_ctx::shrink_stack()
{
    if (nullptr == stack__->stack())
    {
        return;
    }
    ++shrink_stack_counter__;
    if ((shrink_stack_counter__ & CO_SHRINK_STACK_COUNT) != 0)
    {
        shrink_stack_counter__ = 0;
        auto context = reinterpret_cast<const sigcontext_64 *>(&regs__);
        shrink_mem_to_top(reinterpret_cast<co_byte *>(context->sp), stack__->stack());
    }
}

CO_NAMESPACE_END

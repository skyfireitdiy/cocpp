#include "cocpp/sync/co_recursive_mutex.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/utils/co_utils.h"

#include <cassert>

CO_NAMESPACE_BEGIN

void co_recursive_mutex::lock()
{
    CoPreemptGuard();
    auto             ctx = CoCurrentCtx();
    std::scoped_lock lock(spinlock__);

    if (owner__ == nullptr || owner__ == ctx)
    {
        owner__ = ctx;
        ++lock_count__;
        return;
    }

    using namespace std::chrono_literals;
    if (co_timed_call(10ms, [this, ctx] {
            if (owner__ == nullptr)
            {
                owner__ = ctx;
                ++lock_count__;
                return true;
            }
            spinlock__.unlock();
            CoEnablePreempt();
            CoYield();
            CoDisablePreempt();
            spinlock__.lock();
            return false;
        }))
    {
        return;
    }

    if (owner__ == nullptr)
    {
        owner__ = ctx;
        ++lock_count__;
        return;
    }

    wait_deque__.push_back(ctx);
    while (owner__ != ctx)
    {
        ctx->enter_wait_resource_state(this);
        spinlock__.unlock();
        CoEnablePreempt();
        CoYield();
        CoDisablePreempt();
        spinlock__.lock();
    }
}

bool co_recursive_mutex::try_lock()
{
    CoPreemptGuard();
    auto             ctx = CoCurrentCtx();
    std::scoped_lock lock(spinlock__);
    if (owner__ != ctx && owner__ != nullptr)
    {
        return false;
    }
    owner__ = ctx;
    ++lock_count__;
    return true;
}

void co_recursive_mutex::unlock()
{
    CoPreemptGuard();
    auto             ctx = CoCurrentCtx();
    std::scoped_lock lock(spinlock__);
    if (owner__ != ctx)
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, owner__);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    --lock_count__;
    if (lock_count__ != 0)
    {
        return;
    }

    if (wait_deque__.empty())
    {
        owner__ = nullptr;
        return;
    }

    auto obj = wait_deque__.front();
    wait_deque__.pop_front();
    owner__ = obj;
    ++lock_count__;
    obj->leave_wait_resource_state(this);
}

CO_NAMESPACE_END
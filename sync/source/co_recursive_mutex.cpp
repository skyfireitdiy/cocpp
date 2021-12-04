#include "co_recursive_mutex.h"
#include "co.h"
#include "co_define.h"
#include "co_error.h"
#include "co_this_co.h"
#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_recursive_mutex::lock()
{
    auto ctx = co::current_ctx();
    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
    if (owner__ != ctx && owner__ != nullptr)
    {
        ctx->enter_wait_rc_state(CO_RC_TYPE_RECURSIVE_MUTEX, this);
        wait_list__.push_back(ctx);
        do
        {
            spinlock__.unlock(ctx);
            this_co::yield();
            spinlock__.lock(ctx);
        } while (owner__ != ctx && owner__ != nullptr);
    }

    owner__ = ctx;
    ++lock_count__;
}

bool co_recursive_mutex::try_lock()
{
    auto ctx = co::current_ctx();
    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
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
    auto ctx = co::current_ctx();
    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
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

    owner__ = nullptr;
    if (wait_list__.empty())
    {
        return;
    }

    auto next = wait_list__.front();
    wait_list__.pop_front();
    next->leave_wait_rc_state();
}

CO_NAMESPACE_END
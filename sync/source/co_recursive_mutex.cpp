#include "co_recursive_mutex.h"
#include "co.h"
#include "co_define.h"
#include "co_error.h"
#include "co_sync_helper.h"
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
        ctx_enter_wait_state__(ctx, CO_RC_TYPE_RECURSIVE_MUTEX, this, wait_list__);
        lock_yield__(ctx, spinlock__, [this, ctx] { return owner__ != ctx && owner__ != nullptr; });
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
    wake_front__(wait_list__);
}

CO_NAMESPACE_END
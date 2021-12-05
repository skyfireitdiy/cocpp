#include "co_mutex.h"
#include "co.h"
#include "co_defer.h"
#include "co_define.h"
#include "co_error.h"
#include "co_sync_helper.h"
#include "co_this_co.h"

CO_NAMESPACE_BEGIN

void co_mutex::lock()
{
    auto ctx = co::current_ctx();
    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
    if (owner__ != nullptr)
    {
        ctx_enter_wait_state__(ctx, CO_RC_TYPE_MUTEX, this, wait_list__);
        lock_yield__(ctx, spinlock__, [this] { return owner__ != nullptr; });
    }
    owner__ = ctx;
}

bool co_mutex::try_lock()
{
    auto ctx = co::current_ctx();
    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
    if (owner__ != nullptr)
    {
        return false;
    }
    owner__ = ctx;
    return true;
}

void co_mutex::unlock()
{
    auto ctx = co::current_ctx();
    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
    if (owner__ != ctx)
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, owner__);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    owner__ = nullptr;

    wake_front__(wait_list__);
}

CO_NAMESPACE_END
#include "cocpp/sync/co_mutex.h"
#include "cocpp/core/co_define.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/interface/co.h"
#include "cocpp/interface/co_this_co.h"
#include "cocpp/sync/co_sync_helper.h"
#include "cocpp/utils/co_defer.h"
#include <mutex>

CO_NAMESPACE_BEGIN

void co_mutex::lock()
{
    auto             ctx = co::current_ctx();
    std::scoped_lock lock(spinlock__);
    if (owner__ != nullptr)
    {
        ctx_enter_wait_state__(ctx, CO_RC_TYPE_MUTEX, this, wait_deque__);
        lock_yield__(spinlock__, [this] { return owner__ != nullptr; });
    }
    owner__ = ctx;
}

bool co_mutex::try_lock()
{
    auto             ctx = co::current_ctx();
    std::scoped_lock lock(spinlock__);
    if (owner__ != nullptr)
    {
        return false;
    }
    owner__ = ctx;
    return true;
}

void co_mutex::unlock()
{
    auto             ctx = co::current_ctx();
    std::scoped_lock lock(spinlock__);
    if (owner__ != ctx)
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, owner__);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    owner__ = nullptr;

    wake_front__(wait_deque__);
}

CO_NAMESPACE_END
#include "cocpp/sync/co_recursive_mutex.h"
#include "cocpp/core/co_define.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/interface/co.h"
#include "cocpp/interface/co_this_co.h"
#include "cocpp/sync/co_sync_helper.h"
#include <cassert>

CO_NAMESPACE_BEGIN

void co_recursive_mutex::lock()
{
    auto ctx = co::current_ctx();
    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });
    while (owner__ != ctx && owner__ != nullptr)
    {
        ctx_enter_wait_state__(ctx, CO_RC_TYPE_RECURSIVE_MUTEX, this, wait_deque__);
        spinlock__.unlock();
        this_co::yield();
        spinlock__.lock();
    }

    owner__ = ctx;
    ++lock_count__;
}

bool co_recursive_mutex::try_lock()
{
    auto ctx = co::current_ctx();
    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });
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
    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });
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
    wake_front__(wait_deque__);
}

CO_NAMESPACE_END
#include "cocpp/sync/co_recursive_mutex.h"
#include "cocpp/core/co_define.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/interface/co.h"
#include "cocpp/interface/co_this_co.h"

#include <cassert>

CO_NAMESPACE_BEGIN

void co_recursive_mutex::lock()
{
    auto             ctx = co::current_ctx();
    std::scoped_lock lock(spinlock__);

    if (owner__ == nullptr || owner__ == ctx)
    {
        owner__ = ctx;
        ++lock_count__;
        return;
    }

    wait_deque__.push_back(ctx);

    while (owner__ != ctx)
    {
        ctx->enter_wait_resource_state(CO_RC_TYPE_RECURSIVE_MUTEX, this);

        spinlock__.unlock();
        this_co::yield();
        spinlock__.lock();
    }
}

bool co_recursive_mutex::try_lock()
{
    auto             ctx = co::current_ctx();
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
    auto             ctx = co::current_ctx();
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
    obj->leave_wait_resource_state();
}

CO_NAMESPACE_END
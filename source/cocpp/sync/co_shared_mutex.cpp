#include "cocpp/sync/co_shared_mutex.h"
#include "cocpp/core/co_define.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/interface/co.h"
#include "cocpp/interface/co_this_co.h"

#include <algorithm>
#include <cassert>

CO_NAMESPACE_BEGIN

void co_shared_mutex::lock()
{
    auto ctx = co::current_ctx();

    std::scoped_lock lock(spinlock__);

    if (owners__.empty())
    {
        lock_type__ = lock_type::unique;
        owners__.insert(ctx);
        return;
    }

    wait_deque__.push_back(shared_lock_context {
        .type = lock_type::unique,
        .ctx  = ctx });

    while (!owners__.contains(ctx))
    {
        ctx->enter_wait_resource_state(CO_RC_TYPE_SHARED_MUTEX, this);
        spinlock__.unlock();
        this_co::yield();
        spinlock__.lock();
    }
}

bool co_shared_mutex::try_lock()
{
    auto             ctx = co::current_ctx();
    std::scoped_lock lock(spinlock__);
    while (!owners__.empty())
    {
        return false;
    }
    lock_type__ = lock_type::unique;
    owners__.insert(ctx);
    return true;
}

void co_shared_mutex::unlock()
{
    auto ctx = co::current_ctx();

    std::scoped_lock lock(spinlock__);

    if (lock_type__ != lock_type::unique || !owners__.contains(ctx))
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }
    owners__.erase(ctx);
    wake_up_waiters__();
}

void co_shared_mutex::lock_shared()
{
    auto ctx = co::current_ctx();

    std::scoped_lock lock(spinlock__);

    if (lock_type__ == lock_type::shared || lock_type__ == lock_type::unlocked)
    {
        lock_type__ = lock_type::shared;
        owners__.insert(ctx);
        return;
    }

    wait_deque__.push_back(shared_lock_context {
        .type = lock_type::shared,
        .ctx  = ctx });

    while (!owners__.contains(ctx))
    {
        ctx->enter_wait_resource_state(CO_RC_TYPE_SHARED_MUTEX, this);
        spinlock__.unlock();
        this_co::yield();
        spinlock__.lock();
    }
}

bool co_shared_mutex::try_lock_shared()
{
    auto             ctx = co::current_ctx();
    std::scoped_lock lock(spinlock__);
    if (lock_type__ != lock_type::shared && lock_type__ != lock_type::unlocked)
    {
        return false;
    }
    owners__.insert(ctx);
    lock_type__ = lock_type::shared;
    return true;
}

void co_shared_mutex::unlock_shared()
{
    auto             ctx = co::current_ctx();
    std::scoped_lock lock(spinlock__);

    if (lock_type__ != lock_type::shared || !owners__.contains(ctx))
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    owners__.erase(ctx);
    wake_up_waiters__();
}

void co_shared_mutex::wake_up_waiters__()
{
    if (!owners__.empty())
    {
        return;
    }

    lock_type__ = lock_type::unlocked;

    if (wait_deque__.empty())
    {
        return;
    }

    if (wait_deque__.front().type == lock_type::unique)
    {
        auto obj = wait_deque__.front();
        owners__.insert(obj.ctx);
        lock_type__ = lock_type::unique;
        wait_deque__.pop_front();
        obj.ctx->leave_wait_resource_state();
        return;
    }

    auto iter   = std::remove_if(wait_deque__.begin(), wait_deque__.end(), [](auto& c) {
        return c.type == lock_type::shared;
    });
    lock_type__ = lock_type::shared;
    std::transform(iter, wait_deque__.end(), std::inserter(owners__, owners__.begin()), [](auto& context) {
        context.ctx->leave_wait_resource_state();
        return context.ctx;
    });
    wait_deque__.erase(iter, wait_deque__.end());
}

CO_NAMESPACE_END
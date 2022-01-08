#include "cocpp/sync/co_shared_mutex.h"
#include "cocpp/core/co_define.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/interface/co.h"
#include "cocpp/interface/co_this_co.h"
#include "cocpp/sync/co_sync_helper.h"

#include <cassert>

CO_NAMESPACE_BEGIN

void co_shared_mutex::lock()
{
    auto                ctx = co::current_ctx();
    shared_lock_context context {
        .type = lock_type::unique,
        .ctx  = ctx
    };

    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });
    while (!owners__.empty())
    {
        ctx_enter_wait_state__(ctx, CO_RC_TYPE_SHARED_MUTEX, this, wait_deque__, context);
        spinlock__.unlock();
        this_co::yield();
        spinlock__.lock();
    }

    owners__.insert(context);
}

bool co_shared_mutex::try_lock()
{
    auto                ctx = co::current_ctx();
    shared_lock_context context {
        .type = lock_type::unique,
        .ctx  = ctx
    };

    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });
    while (!owners__.empty())
    {
        return false;
    }
    owners__.insert(context);
    return true;
}

void co_shared_mutex::unlock()
{
    auto                ctx = co::current_ctx();
    shared_lock_context context {
        .type = lock_type::unique,
        .ctx  = ctx
    };

    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });

    if (!owners__.contains(context))
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    owners__.erase(context);
    wake_up_waiters__();
}

void co_shared_mutex::lock_shared()
{
    auto                ctx = co::current_ctx();
    shared_lock_context context {
        .type = lock_type::shared,
        .ctx  = ctx
    };

    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });
    while (!owners__.empty() && (*owners__.begin()).type == lock_type::unique)
    {
        ctx_enter_wait_state__(ctx, CO_RC_TYPE_SHARED_MUTEX, this, wait_deque__, context);

        spinlock__.unlock();
        this_co::yield();
        spinlock__.lock();
    }

    owners__.insert(context);
}

bool co_shared_mutex::try_lock_shared()
{
    auto                ctx = co::current_ctx();
    shared_lock_context context {
        .type = lock_type::shared,
        .ctx  = ctx
    };

    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });
    if (!owners__.empty() && (*owners__.begin()).type == lock_type::unique)
    {
        return false;
    }
    owners__.insert(context);
    return true;
}

void co_shared_mutex::unlock_shared()
{
    auto                ctx = co::current_ctx();
    shared_lock_context context {
        .type = lock_type::shared,
        .ctx  = ctx
    };

    spinlock__.lock();
    CoDefer([this] { spinlock__.unlock(); });

    if (!owners__.contains(context))
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    owners__.erase(context);
    wake_up_waiters__();
}

void co_shared_mutex::wake_up_waiters__()
{
    if (wait_deque__.empty())
    {
        return;
    }

    if (wait_deque__.front().type == lock_type::unique)
    {
        wake_front__(wait_deque__, std::function([](shared_lock_context& ctx) {
                         ctx.ctx->leave_wait_resource_state();
                     }));
        return;
    }

    auto iter = std::remove_if(wait_deque__.begin(), wait_deque__.end(), [](auto& c) {
        return c.type == lock_type::shared;
    });

    std::deque<shared_lock_context> new_owner { iter, wait_deque__.end() };
    wait_deque__.erase(iter, wait_deque__.end());
    for (auto& c : new_owner)
    {
        c.ctx->leave_wait_resource_state();
    }
}

bool co_shared_mutex::shared_lock_context::operator==(const co_shared_mutex::shared_lock_context& other) const
{
    return ctx == other.ctx && type == other.type;
}

std::size_t co_shared_mutex::lock_context_hasher::operator()(const shared_lock_context& other) const
{
    return std::hash<co_ctx*> {}(other.ctx) ^ (std::hash<lock_type> {}(other.type) << 1);
}

CO_NAMESPACE_END
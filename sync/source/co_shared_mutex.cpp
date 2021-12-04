#include "co_shared_mutex.h"
#include "co.h"
#include "co_error.h"
#include "co_sync_helper.h"
#include "co_this_co.h"

#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_shared_mutex::lock()
{
    auto         ctx = co::current_ctx();
    lock_context context {
        .type = lock_type::unique,
        .ctx  = ctx
    };

    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
    if (!owners__.empty())
    {
        ctx->enter_wait_rc_state(CO_RC_TYPE_SHARED_MUTEX, this);
        wait_list__.push_back(context);

        lock_yield__(ctx, spinlock__, [this] { return !owners__.empty(); });
    }

    owners__.insert(context);
}

bool co_shared_mutex::try_lock()
{
    auto         ctx = co::current_ctx();
    lock_context context {
        .type = lock_type::unique,
        .ctx  = ctx
    };

    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
    while (!owners__.empty())
    {
        return false;
    }
    owners__.insert(context);
    return true;
}

void co_shared_mutex::unlock()
{
    auto         ctx = co::current_ctx();
    lock_context context {
        .type = lock_type::unique,
        .ctx  = ctx
    };

    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });

    if (!owners__.contains(context))
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    owners__.erase(context);

    if (wait_list__.empty())
    {
        return;
    }

    if (wait_list__.front().type == lock_type::unique)
    {
        auto next = wait_list__.front();
        wait_list__.pop_front();
        next.ctx->leave_wait_rc_state();
        return;
    }

    auto iter = std::remove_if(wait_list__.begin(), wait_list__.end(), [](auto& c) {
        return c.type == lock_type::shared;
    });

    std::list<lock_context> new_owner { iter, wait_list__.end() };
    wait_list__.erase(iter, wait_list__.end());
    for (auto& c : new_owner)
    {
        c.ctx->leave_wait_rc_state();
    }
}

void co_shared_mutex::lock_shared()
{
    auto         ctx = co::current_ctx();
    lock_context context {
        .type = lock_type::shared,
        .ctx  = ctx
    };

    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
    if (!owners__.empty() && (*owners__.begin()).type == lock_type::unique)
    {
        ctx->enter_wait_rc_state(CO_RC_TYPE_SHARED_MUTEX, this);
        wait_list__.push_back(context);

        lock_yield__(ctx, spinlock__, [this] { return !owners__.empty() && (*owners__.begin()).type == lock_type::unique; });
    }

    owners__.insert(context);
}

bool co_shared_mutex::try_lock_shared()
{
    auto         ctx = co::current_ctx();
    lock_context context {
        .type = lock_type::shared,
        .ctx  = ctx
    };

    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });
    if (!owners__.empty() && (*owners__.begin()).type == lock_type::unique)
    {
        return false;
    }
    owners__.insert(context);
    return true;
}

void co_shared_mutex::unlock_shared()
{
    auto         ctx = co::current_ctx();
    lock_context context {
        .type = lock_type::shared,
        .ctx  = ctx
    };

    spinlock__.lock(ctx);
    CoDefer([this, ctx] { spinlock__.unlock(ctx); });

    if (!owners__.contains(context))
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    owners__.erase(context);

    if (wait_list__.empty())
    {
        return;
    }

    if (wait_list__.front().type == lock_type::unique)
    {
        auto next = wait_list__.front();
        wait_list__.pop_front();
        next.ctx->leave_wait_rc_state();
        return;
    }

    auto iter = std::remove_if(wait_list__.begin(), wait_list__.end(), [](auto& c) {
        return c.type == lock_type::shared;
    });

    std::list<lock_context> new_owner { iter, wait_list__.end() };
    wait_list__.erase(iter, wait_list__.end());
    for (auto& c : new_owner)
    {
        c.ctx->leave_wait_rc_state();
    }
}

bool co_shared_mutex::lock_context::operator==(const co_shared_mutex::lock_context& other) const
{
    return ctx == other.ctx && type == other.type;
}

std::size_t co_shared_mutex::lock_context_hasher::operator()(const lock_context& other) const
{
    return std::hash<co_ctx*> {}(other.ctx) ^ (std::hash<lock_type> {}(other.type) << 1);
}

CO_NAMESPACE_END
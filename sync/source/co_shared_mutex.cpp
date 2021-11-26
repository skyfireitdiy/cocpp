#include "co_shared_mutex.h"
#include "co.h"
#include "co_error.h"
#include "co_sync_utils.h"
#include "co_this_co.h"

#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_shared_mutex::lock()
{
    auto ctx = co::current_ctx();

    lock_context context {
        lock_type::unique,
        ctx
    };

    while (!try_lock())
    {
        add_to_wait_list<lock_context>(wait_list__, context, spinlock__);
        ctx->set_wait_flag(CO_RC_TYPE_SHARED_MUTEX, this);
        co::current_env()->schedule_switch(true);
    }
}

bool co_shared_mutex::try_lock()
{
    auto ctx = co::current_ctx();

    lock_context context {
        lock_type::unique,
        ctx
    };

    std::unique_lock<co_spinlock> lck(spinlock__);
    if (owners__.empty())
    {
        owners__.push_back(context);
        wait_list__.remove(context);
        return true;
    }
    return false;
}

void co_shared_mutex::unlock()
{
    auto         ctx = co::current_ctx();
    lock_context context {
        lock_type::unique,
        ctx
    };

    std::unique_lock<co_spinlock> lck(spinlock__);
    owners__.remove(context);
    if (owners__.empty())
    {
        wakeup_all_ctx<lock_context>(wait_list__, [](const lock_context& c) -> co_ctx* {
            return c.ctx;
        });
    }
}

void co_shared_mutex::lock_shared()
{
    auto ctx = co::current_ctx();

    lock_context context {
        lock_type::shared,
        ctx
    };
    while (!try_lock_shared())
    {
        add_to_wait_list(wait_list__, context, spinlock__);
        ctx->set_wait_flag(CO_RC_TYPE_SHARED_MUTEX, this);
        co::current_env()->schedule_switch(true);
    }
}

bool co_shared_mutex::try_lock_shared()
{
    auto ctx = co::current_ctx();

    lock_context context {
        lock_type::shared,
        ctx
    };

    std::unique_lock<co_spinlock> lck(spinlock__);
    if (owners__.empty())
    {
        owners__.push_back(context);
        wait_list__.remove(context);
        return true;
    }

    auto curr_type = owners__.front().type;
    if (curr_type == lock_type::shared)
    {
        owners__.push_back(context);
        wait_list__.remove(context);
        return true;
    }

    return false;
}

void co_shared_mutex::unlock_shared()
{
    auto         ctx = co::current_ctx();
    lock_context context {
        lock_type::shared,
        ctx
    };

    std::unique_lock<co_spinlock> lck(spinlock__);
    owners__.remove(context);
    if (owners__.empty())
    {
        wakeup_all_ctx<lock_context>(wait_list__, [](const lock_context& c) -> co_ctx* {
            return c.ctx;
        });
    }
}

bool co_shared_mutex::lock_context::operator==(const co_shared_mutex::lock_context& other) const
{
    return ctx == other.ctx && type == other.type;
}

CO_NAMESPACE_END
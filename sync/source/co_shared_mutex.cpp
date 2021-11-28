#include "co_shared_mutex.h"
#include "co.h"
#include "co_error.h"
#include "co_this_co.h"

#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_shared_mutex::lock()
{
    while (!try_lock())
    {
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
    if (auto iter = std::find(owners__.begin(), owners__.end(), context); iter == owners__.end())
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }
    else
    {
        owners__.erase(iter);
    }
}

void co_shared_mutex::lock_shared()
{
    while (!try_lock_shared())
    {
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

        return true;
    }

    auto curr_type = owners__.front().type;
    if (curr_type == lock_type::shared)
    {
        owners__.push_back(context);

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
    if (auto iter = std::find(owners__.begin(), owners__.end(), context); iter == owners__.end())
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p", ctx);
        throw co_error("ctx is not owner[", ctx, "]");
    }
    else
    {
        owners__.erase(iter);
    }
}

bool co_shared_mutex::lock_context::operator==(const co_shared_mutex::lock_context& other) const
{
    return ctx == other.ctx && type == other.type;
}

CO_NAMESPACE_END
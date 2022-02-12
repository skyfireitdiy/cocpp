#include "cocpp/sync/co_mutex.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/utils/co_defer.h"
#include "cocpp/utils/co_utils.h"
#include <chrono>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_mutex::lock()
{
    auto             ctx = co_manager::instance()->current_env()->current_ctx();
    std::scoped_lock lock(spinlock__);

    if (owner__ == nullptr)
    {
        owner__ = ctx;
        return;
    }

    using namespace std::chrono_literals;
    auto deadline = std::chrono::steady_clock::now() + 10ms;
    while (std::chrono::steady_clock::now() < deadline)
    {
        spinlock__.unlock();
        co_manager::instance()->current_env()->schedule_switch();
        spinlock__.lock();
        if (owner__ == nullptr)
        {
            owner__ = ctx;
            return;
        }
    }

    using namespace std::chrono_literals;
    if (co_timed_call(10ms, [this, ctx] {
            if (owner__ == nullptr)
            {
                owner__ = ctx;
                return true;
            }
            spinlock__.unlock();
            co_manager::instance()->current_env()->schedule_switch();
            spinlock__.lock();
            return false;
        }))
    {
        return;
    }

    wait_deque__.push_back(ctx);

    while (owner__ != ctx)
    {
        ctx->enter_wait_resource_state(CO_RC_TYPE_MUTEX, this);
        spinlock__.unlock();
        co_manager::instance()->current_env()->schedule_switch();
        spinlock__.lock();
    }
    owner__ = ctx;
}

bool co_mutex::try_lock()
{
    auto             ctx = co_manager::instance()->current_env()->current_ctx();
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
    auto             ctx = co_manager::instance()->current_env()->current_ctx();
    std::scoped_lock lock(spinlock__);
    if (owner__ != ctx)
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, owner__);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    if (wait_deque__.empty())
    {
        owner__ = nullptr;
        return;
    }

    auto obj = wait_deque__.front();
    wait_deque__.pop_front();
    owner__ = obj;
    obj->leave_wait_resource_state();
}

CO_NAMESPACE_END
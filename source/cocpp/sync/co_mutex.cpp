#include "cocpp/sync/co_mutex.h"
#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/exception/co_error.h"
#include "cocpp/utils/co_defer.h"
#include "cocpp/utils/co_utils.h"
#include <atomic>
#include <chrono>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_mutex::lock()
{
    auto ctx = CoCurrentCtx();

    std::scoped_lock lock(spinlock__);

    // Try to get the lock by spin
    using namespace std::chrono_literals;
    if (co_timed_call(10ms, [this, ctx] {
            // At this time, nullPTR is preempted because it is not in the wait queue
            if (owner__ == nullptr)
            {
                owner__ = ctx;
                return true;
            }

            // Other coroutine unlocks require a spin lock
            spinlock__.unlock();
            CoYield();
            spinlock__.lock();
            return false;
        }))
    {

        return;
    }

    // It's unlocked when you switch back
    if (owner__ == nullptr)
    {
        owner__ = ctx;
        return;
    }

    // If the lock is not obtained, the current coroutine is added to the wait queue
    wait_deque__.push_back(ctx);

    while (owner__ != ctx)
    {
        // Set the wait state so that the scheduling module does not schedule this coroutine
        ctx->enter_wait_resource_state(this);
        spinlock__.unlock();
        CoYield();

        // This coroutine is rescheduled, so coroutine unlocking removes the wait flag for the current coroutine
        spinlock__.lock();
    }
}

bool co_mutex::try_lock()
{
    auto             ctx = CoCurrentCtx();
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
    auto             ctx = CoCurrentCtx();
    std::scoped_lock lock(spinlock__);
    if (owner__ != ctx)
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, owner__);
        throw co_error("ctx is not owner[", ctx, "]");
    }

    // Wait queue is empty, return with owner set to nullptr
    if (wait_deque__.empty())
    {
        owner__ = nullptr;
        return;
    }

    // The wait queue is not empty, set owner to first in the wait queue, and wake up
    auto obj = wait_deque__.front();
    wait_deque__.pop_front();
    owner__ = obj;
    obj->leave_wait_resource_state(this);
}

CO_NAMESPACE_END
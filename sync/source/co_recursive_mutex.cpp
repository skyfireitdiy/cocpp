#include "co_recursive_mutex.h"
#include "co.h"
#include "co_error.h"
#include "co_this_co.h"
#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_recursive_mutex::lock()
{
    while (!try_lock())
    {
        co::current_env()->schedule_switch(true);
    }
}

bool co_recursive_mutex::try_lock()
{
    auto ctx = co::current_ctx();

    std::lock_guard<co_mutex> lck(spinlock__);
    if (owner__ == nullptr || owner__ == ctx)
    {
        owner__ = ctx;
        ++lock_count__;
        return true;
    }
    return false;
}

void co_recursive_mutex::unlock()
{
    auto ctx = co::current_ctx();

    std::lock_guard<co_mutex> lck(spinlock__);
    if (owner__ != ctx) // 当前ctx不是加锁ctx，异常
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, owner__.load());
        throw co_error("ctx is not owner[", ctx, "]");
    }

    --lock_count__;
    if (lock_count__ != 0) // 还有上层锁
    {
        return;
    }

    owner__ = nullptr;
}

CO_NAMESPACE_END
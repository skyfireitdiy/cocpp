#include "co_recursive_mutex.h"
#include "co.h"
#include "co_error.h"
#include "co_sync_utils.h"
#include "co_this_co.h"
#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_recursive_mutex::lock()
{
    auto ctx = co::current_ctx();
    while (!try_lock())
    {
        add_to_wait_list<co_ctx*>(waited_ctx_list__, ctx, spinlock__);
        ctx->set_wait_flag(CO_RC_TYPE_RECURSIVE_MUTEX, this);
        co::current_env()->schedule_switch(true);
    }
}

bool co_recursive_mutex::try_lock()
{
    auto ctx = co::current_ctx();

    std::lock_guard<co_spinlock> lck(spinlock__);
    if (owner__ == nullptr || owner__ == ctx)
    {
        owner__ = ctx;
        ++lock_count__;
        waited_ctx_list__.remove(ctx);
        return true;
    }
    return false;
}

void co_recursive_mutex::unlock()
{
    auto ctx = co::current_ctx();

    std::lock_guard<co_spinlock> lck(spinlock__);
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
    wakeup_one_ctx<co_ctx*>(waited_ctx_list__, [](co_ctx* const c) -> co_ctx* { return c; });
}

CO_NAMESPACE_END
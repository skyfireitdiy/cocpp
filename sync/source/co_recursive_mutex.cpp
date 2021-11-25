#include "co_recursive_mutex.h"
#include "co.h"
#include "co_error.h"
#include "co_this_co.h"
#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_recursive_mutex::lock()
{
    auto ctx = co::current_ctx();

    std::unique_lock<co_spinlock> lck(spinlock__);
    if (owner__ == nullptr || owner__ == ctx)
    {
        owner__ = ctx;
        ++lock_count__;
        return;
    }

    // 加入等待队列
    ctx->set_flag(CO_CTX_FLAG_WAITING);
    waited_ctx_list__.push_back(ctx);

    while (owner__ != ctx)
    {
        lck.unlock();
        co::current_env()->schedule_switch(true);
        lck.lock();
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

    if (waited_ctx_list__.empty()) // 没有等待者了
    {
        owner__ = nullptr;
        return;
    }

    auto waked_ctx = waited_ctx_list__.front();
    waited_ctx_list__.pop_front();
    owner__ = waked_ctx;
    ++lock_count__;

    assert(waked_ctx != nullptr);
    // 状态设置为suspend，此状态可调度
    waked_ctx->reset_flag(CO_CTX_FLAG_WAITING);
    // 唤醒对应的env
    waked_ctx->env()->wake_up();
}

CO_NAMESPACE_END
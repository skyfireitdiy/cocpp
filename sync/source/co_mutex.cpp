#include "co_mutex.h"
#include "co.h"
#include "co_define.h"
#include "co_error.h"
#include "co_this_co.h"
#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

void co_mutex::lock()
{
    auto ctx = co::current_ctx();

    std::unique_lock<co_spinlock> lck(spinlock__);
    if (owner__ == nullptr) // 锁处于空闲状态
    {
        // CO_O_DEBUG("set owner %p", ctx);
        owner__ = ctx;
        return;
    }

    ctx->set_flag(CO_CTX_FLAG_WAITING); // 设置等待标记
    // CO_O_DEBUG("add to wait list %p", ctx);
    waited_ctx_list__.push_back(ctx); // 添加到等待队列

    while (owner__ != ctx) // 被唤醒的有可能是idle ctx
    {
        lck.unlock();
        co::current_env()->schedule_switch(false);
        lck.lock();
    }
}

bool co_mutex::try_lock()
{
    auto ctx = co::current_ctx();

    std::lock_guard<co_spinlock> lck(spinlock__);
    if (owner__ == nullptr)
    {
        // CO_O_DEBUG("set owner %p", ctx);
        owner__ = ctx;
        return true;
    }
    return false;
}

void co_mutex::unlock()
{
    auto ctx = co::current_ctx();

    std::lock_guard<co_spinlock> lck(spinlock__);
    if (owner__ != ctx) // 当前ctx不是加锁ctx，异常
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, owner__.load());
        throw co_error("ctx is not owner[", ctx, "]");
    }

    // 没有等待ctx，设置拥有者为nullptr
    if (waited_ctx_list__.empty())
    {
        // CO_O_DEBUG("set owner nullptr");
        owner__ = nullptr;
        return;
    }

    // 锁拥有者设置为等待队列中第一个
    auto waked_ctx = waited_ctx_list__.front();
    waited_ctx_list__.pop_front();
    // CO_O_DEBUG("set owner %p", waked_ctx);
    owner__ = waked_ctx;

    assert(waked_ctx != nullptr);
    // 状态设置为suspend，此状态可调度
    waked_ctx->reset_flag(CO_CTX_FLAG_WAITING);
    // 唤醒对应的env
    waked_ctx->env()->wake_up();
}

CO_NAMESPACE_END
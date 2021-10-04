#include "co_mutex.h"
#include "co.h"
#include "co_error.h"
#include <mutex>

void co_mutex::lock()
{
    auto ctx = co::manager__->current_env()->current_ctx();

    spinlock__.lock();
    if (owner__ == nullptr) // 锁处于空闲状态
    {
        owner__ = ctx;
        CO_O_DEBUG("set owner: %p", owner__.load());
        spinlock__.unlock();
        return;
    }

    // CO_O_DEBUG("add to wait list: %p", ctx);
    ctx->set_state(co_state::waitting); // 设置当前状态为等待状态
    waited_ctx_list__.push_back(ctx);   // 添加到等待队列
    spinlock__.unlock();                // 需要在此处解锁，否则切换出去的时候无法解锁
    while (owner__ != ctx)              // 被唤醒的有可能是idle ctx
    {
        co::schedule_switch(); // 再次切换回来的时候说明已经获得了锁
    }
    // CO_O_DEBUG("ctx %p get lock", ctx);
}

bool co_mutex::try_lock()
{
    auto ctx = co::manager__->current_env()->current_ctx();
    spinlock__.lock();
    if (owner__ == nullptr)
    {
        owner__ = ctx;
        CO_O_DEBUG("set owner: %p", owner__.load());
        spinlock__.unlock();
        return true;
    }
    spinlock__.unlock();
    return false;
}

void co_mutex::unlock()
{
    auto ctx = co::manager__->current_env()->current_ctx();

    spinlock__.lock();

    if (owner__ != ctx) // 当前ctx不是加锁ctx，异常
    {
        CO_O_ERROR("ctx is not owner, this ctx is %p, owner is %p", ctx, owner__.load());
        throw co_error("ctx is not owner[", ctx, "]");
    }

    // 没有等待ctx，设置拥有者为nullptr
    if (waited_ctx_list__.empty())
    {
        owner__ = nullptr;
        CO_O_DEBUG("set owner: nullptr");
        spinlock__.unlock();
        return;
    }

    // 锁拥有者设置为等待队列中第一个
    auto waked_ctx = waited_ctx_list__.front();
    waited_ctx_list__.pop_front();
    owner__ = waked_ctx;
    CO_O_DEBUG("set owner: %p from %p", waked_ctx, ctx);
    spinlock__.unlock();

    // 状态设置为suspend，此状态可调度
    waked_ctx->set_state(co_state::suspended);
    // 唤醒对应的env
    waked_ctx->env()->wake_up();
}
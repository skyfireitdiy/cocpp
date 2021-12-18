#include "co_scheduler.h"
#include "co_ctx.h"
#include "co_define.h"
#include "co_entry.h"
#include <cassert>
#include <list>

CO_NAMESPACE_BEGIN

co_scheduler::co_scheduler()
    : all_scheduleable_ctx__(CO_MAX_PRIORITY)
{
}

// void co_scheduler::add_ctx(co_ctx* ctx)
// {
//     std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);
//     all_scheduleable_ctx__[ctx->priority()].push_back(ctx);
//     update_min_priority__(ctx->priority());
//     // CO_O_DEBUG("add ctx %s %p , state: %d\n", ctx->config().name.c_str(), ctx, (int)ctx->state());
// }

void co_scheduler::remove_ctx(co_ctx* ctx)
{
    std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);
    // CO_O_DEBUG("remove ctx %s %p , state: %d", ctx->config().name.c_str(), ctx, (int)ctx->state());
    // 此处不能断言 curr__ != ctx，因为在最后清理所有的ctx的时候，可以删除当前ctx
    all_scheduleable_ctx__[ctx->priority()].remove(ctx);
}

co_ctx* co_scheduler::choose_ctx()
{
    std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);
    for (unsigned int i = min_priority__; i < all_scheduleable_ctx__.size(); ++i)
    {
        for (auto& ctx : all_scheduleable_ctx__[i])
        {
            if (ctx->can_schedule())
            {
                auto ret = ctx;
                all_scheduleable_ctx__[i].remove(ctx);
                all_scheduleable_ctx__[i].push_back(ret);
                curr_obj__     = ret;
                min_priority__ = i;
                return ret;
            }
        }
    }
    curr_obj__ = nullptr;
    return nullptr;
}

std::list<co_ctx*> co_scheduler::all_ctx() const
{
    std::scoped_lock   lock(mu_scheduleable_ctx__, mu_blocked_ctx__);
    std::list<co_ctx*> ret;
    for (auto& lst : all_scheduleable_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    ret.insert(ret.begin(), blocked_ctx__.begin(), blocked_ctx__.end());
    return ret;
}

std::list<co_ctx*> co_scheduler::all_scheduleable_ctx() const
{
    std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);
    std::list<co_ctx*>           ret;
    for (auto& lst : all_scheduleable_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    return ret;
}

size_t co_scheduler::count() const
{
    std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);

    size_t ret = 0;
    for (unsigned int i = min_priority__; i < all_scheduleable_ctx__.size(); ++i)
    {
        ret += all_scheduleable_ctx__[i].size();
    }
    return ret;
}

co_ctx* co_scheduler::current_ctx() const
{
    std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);

    return curr_obj__;
}

bool co_scheduler::can_schedule() const
{
    std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);
    for (unsigned int i = min_priority__; i < all_scheduleable_ctx__.size(); ++i)
    {
        for (auto& ctx : all_scheduleable_ctx__[i])
        {
            if (ctx->can_schedule())
            {
                return true;
            }
        }
    }
    return false;
}

void co_scheduler::change_priority(int old, co_ctx* ctx)
{
    std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);
    for (auto iter = all_scheduleable_ctx__[old].begin(); iter != all_scheduleable_ctx__[old].end(); ++iter)
    {
        if (*iter == ctx)
        {
            all_scheduleable_ctx__[old].erase(iter);
            all_scheduleable_ctx__[ctx->priority()].push_back(ctx);
            update_min_priority__(ctx->priority());
            return;
        }
    }
    assert(false);
}

void co_scheduler::ctx_leave_wait_state(co_ctx* ctx)
{
    std::scoped_lock lock(mu_scheduleable_ctx__, mu_blocked_ctx__);
    blocked_ctx__.erase(ctx);
    all_scheduleable_ctx__[ctx->priority()].push_back(ctx);
    update_min_priority__(ctx->priority());
}

void co_scheduler::update_min_priority__(int priority)
{
    if (priority < min_priority__)
    {
        min_priority__ = priority;
    }
}

void co_scheduler::ctx_enter_wait_state(co_ctx* ctx)
{
    std::scoped_lock lock(mu_scheduleable_ctx__, mu_blocked_ctx__);
    all_scheduleable_ctx__[ctx->priority()].remove(ctx);
    blocked_ctx__.insert(ctx);
}

std::list<co_ctx*> co_scheduler::take_all_movable_ctx()
{
    std::lock_guard<co_spinlock> lock(mu_scheduleable_ctx__);
    std::list<co_ctx*>           ret;
    for (unsigned int i = min_priority__; i < all_scheduleable_ctx__.size(); ++i)
    {
        auto backup = all_scheduleable_ctx__[i];
        for (auto& ctx : backup)
        {
            if (ctx->can_move())
            {
                all_scheduleable_ctx__[i].remove(ctx);
                ret.push_back(ctx);
            }
        }
    }
    return ret;
}

CO_NAMESPACE_END
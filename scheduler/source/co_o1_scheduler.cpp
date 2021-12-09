#include "co_o1_scheduler.h"
#include "co_ctx.h"
#include "co_define.h"
#include "co_entry.h"
#include <cassert>
#include <mutex>

CO_NAMESPACE_BEGIN

co_o1_scheduler::co_o1_scheduler()
    : all_ctx__(CO_MAX_PRIORITY)
{
}

void co_o1_scheduler::add_ctx(co_ctx* ctx)
{
    std::lock_guard<co_spinlock> lock(mu_all_ctx__);
    all_ctx__[ctx->priority()].push_back(ctx);
    update_min_priority__(ctx->priority());
    // CO_O_DEBUG("add ctx %s %p , state: %d\n", ctx->config().name.c_str(), ctx, (int)ctx->state());
}

void co_o1_scheduler::remove_ctx(co_ctx* ctx)
{
    std::lock_guard<co_spinlock> lock(mu_all_ctx__);
    // CO_O_DEBUG("remove ctx %s %p , state: %d", ctx->config().name.c_str(), ctx, (int)ctx->state());
    // 此处不能断言 curr__ != ctx，因为在最后清理所有的ctx的时候，可以删除当前ctx
    all_ctx__[ctx->priority()].remove(ctx);
}

co_ctx* co_o1_scheduler::choose_ctx()
{
    std::lock_guard<co_spinlock> lock(mu_all_ctx__);
    for (unsigned int i = min_priority__; i < all_ctx__.size(); ++i)
    {
        if (all_ctx__[i].empty())
        {
            continue;
        }

        for (auto& ctx : all_ctx__[i])
        {
            if (ctx->can_schedule())
            {
                auto ret = ctx;
                all_ctx__[i].remove(ctx);
                all_ctx__[i].push_back(ret);
                curr_obj__     = ret;
                min_priority__ = i;
                return ret;
            }
        }
    }
    curr_obj__ = nullptr;
    return nullptr;
}

std::list<co_ctx*> co_o1_scheduler::all_ctx() const
{
    std::lock_guard<co_spinlock> lock(mu_all_ctx__);
    std::list<co_ctx*>           ret;
    for (auto& lst : all_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    return ret;
}

size_t co_o1_scheduler::count() const
{
    std::lock_guard<co_spinlock> lock(mu_all_ctx__);

    size_t ret = 0;
    for (unsigned int i = min_priority__; i < all_ctx__.size(); ++i)
    {
        ret += all_ctx__[i].size();
    }
    return ret;
}

co_ctx* co_o1_scheduler::current_ctx() const
{
    std::lock_guard<co_spinlock> lock(mu_all_ctx__);

    return curr_obj__;
}

bool co_o1_scheduler::can_schedule() const
{
    std::lock_guard<co_spinlock> lock(mu_all_ctx__);
    for (unsigned int i = min_priority__; i < all_ctx__.size(); ++i)
    {
        for (auto& ctx : all_ctx__[i])
        {
            if (ctx->can_schedule())
            {
                return true;
            }
        }
    }
    return false;
}

void co_o1_scheduler::change_priority(int old, co_ctx* ctx)
{
    std::lock_guard<co_spinlock> lock(mu_all_ctx__);
    for (auto iter = all_ctx__[old].begin(); iter != all_ctx__[old].end(); ++iter)
    {
        if (*iter == ctx)
        {
            all_ctx__[old].erase(iter);
            all_ctx__[ctx->priority()].push_back(ctx);
            update_min_priority__(ctx->priority());
            return;
        }
    }
    assert(false);
}

void co_o1_scheduler::ctx_wake_up(co_ctx* ctx)
{
    update_min_priority__(ctx->priority());
}

void co_o1_scheduler::update_min_priority__(int priority)
{
    if (priority < min_priority__)
    {
        min_priority__ = priority;
    }
}

CO_NAMESPACE_END
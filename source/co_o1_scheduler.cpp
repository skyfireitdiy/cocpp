#include "co_o1_scheduler.h"
#include "co_ctx.h"
#include "co_define.h"
#include <cassert>
#include <mutex>

co_o1_scheduler::co_o1_scheduler()
    : all_ctx__(CO_MAX_PRIORITY)
{
}

void co_o1_scheduler::add_ctx(co_ctx* ctx)
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    all_ctx__[ctx->priority()].push_back(ctx);
    if (ctx->priority() < min_priority__)
    {
        min_priority__ = ctx->priority();
    }
    // CO_DEBUG("add ctx %s %p , state: %d\n", ctx->config().name.c_str(), ctx, ctx->state());
}

void co_o1_scheduler::remove_ctx(co_ctx* ctx)
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    // CO_DEBUG("remove ctx %s %p , state: %d\n", ctx->config().name.c_str(), ctx, ctx->state());
    assert(ctx != curr__);
    all_ctx__[ctx->priority()].remove(ctx);
}

co_ctx* co_o1_scheduler::choose_ctx()
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    for (int i = min_priority__; i < all_ctx__.size(); ++i)
    {
        if (all_ctx__[i].empty())
        {
            continue;
        }

        for (auto& ctx : all_ctx__[i])
        {
            if (ctx->state() != co_state::finished)
            {
                auto ret = ctx;
                all_ctx__[i].remove(ctx);
                all_ctx__[i].push_back(ret);
                curr__         = ret;
                min_priority__ = i;
                return ret;
            }
        }
    }
    curr__ = nullptr;
    return nullptr;
}

std::list<co_ctx*> co_o1_scheduler::all_ctx() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    std::list<co_ctx*>          ret;
    for (int i = 0; i < all_ctx__.size(); ++i)
    {
        ret.insert(ret.end(), all_ctx__[i].begin(), all_ctx__[i].end());
    }
    return ret;
}

size_t co_o1_scheduler::count() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);

    size_t ret = 0;
    for (int i = 0; i < all_ctx__.size(); ++i)
    {
        ret += all_ctx__[i].size();
    }
    return ret;
}

co_ctx* co_o1_scheduler::current_ctx() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);

    return curr__;
}
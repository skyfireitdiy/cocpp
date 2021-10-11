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
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    all_ctx__[ctx->priority()].push_back(ctx);
    // CO_O_DEBUG("add ctx %s %p , state: %d\n", ctx->config().name.c_str(), ctx, (int)ctx->state());
}

void co_o1_scheduler::remove_ctx(co_ctx* ctx)
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    // CO_O_DEBUG("remove ctx %s %p , state: %d", ctx->config().name.c_str(), ctx, (int)ctx->state());
    // 此处不能断言 curr__ != ctx，因为在最后清理所有的ctx的时候，可以删除当前ctx
    all_ctx__[ctx->priority()].remove(ctx);
}

co_ctx* co_o1_scheduler::choose_ctx()
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    for (unsigned int i = 0; i < all_ctx__.size(); ++i)
    {
        if (all_ctx__[i].empty())
        {
            continue;
        }

        for (auto& ctx : all_ctx__[i])
        {
            if (ctx_can_schedule__(ctx))
            {
                auto ret = ctx;
                all_ctx__[i].remove(ctx);
                all_ctx__[i].push_back(ret);
                curr__ = ret;
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
    for (auto& lst : all_ctx__)
    {
        ret.insert(ret.end(), lst.begin(), lst.end());
    }
    return ret;
}

size_t co_o1_scheduler::count() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);

    size_t ret = 0;
    for (unsigned int i = 0; i < all_ctx__.size(); ++i)
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

bool co_o1_scheduler::can_schedule() const
{
    std::lock_guard<std::mutex> lock(mu_all_ctx__);
    for (unsigned int i = 0; i < all_ctx__.size(); ++i)
    {
        for (auto& ctx : all_ctx__[i])
        {
            if (ctx_can_schedule__(ctx))
            {
                return true;
            }
        }
    }
    return false;
}

bool co_o1_scheduler::ctx_can_schedule__(co_ctx* ctx)
{
    return ctx->state() != co_state::finished && !ctx->test_flag(CO_CTX_FLAG_WAITING);
}

CO_NAMESPACE_END
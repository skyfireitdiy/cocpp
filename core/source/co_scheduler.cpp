#include "co_scheduler.h"
#include "co_ctx.h"
#include "co_define.h"

#include <algorithm>

CO_NAMESPACE_BEGIN

void co_scheduler::add_ctx(co_ctx* ctx)
{
    std::lock_guard<co_spinlock> lock(all_normal_ctx_lock__);
    all_normal_ctx__[ctx->priority()].push_back(ctx);
    ++count__;
    min_priority__ = std::min(min_priority__, ctx->priority());
}

co_ctx* co_scheduler::choose_next_ctx()
{
    std::lock_guard<co_spinlock> lock(all_normal_ctx_lock__);
    for (auto priority = min_priority__; priority < CO_MAX_PRIORITY; ++priority)
    {
        for (auto ctx : all_normal_ctx__[priority])
        {
            all_normal_ctx__[priority].remove(ctx);
            min_priority__ = priority;
            --count__;
            return ctx;
        }
    }
    return nullptr;
}

size_t co_scheduler::min_priority()
{
    std::lock_guard<co_spinlock> lock(all_normal_ctx_lock__);
    return min_priority__;
}

size_t co_scheduler::count()
{
    std::lock_guard<co_spinlock> lock(all_normal_ctx_lock__);
    return count__;
}

std::list<co_ctx*> co_scheduler::all_ctx()
{
    std::lock_guard<co_spinlock> lock(all_normal_ctx_lock__);
    std::list<co_ctx*>           result;
    for (auto& list : all_normal_ctx__)
    {
        result.splice(result.end(), list);
        list.clear();
    }
    count__ = 0;
    return result;
}

CO_NAMESPACE_END
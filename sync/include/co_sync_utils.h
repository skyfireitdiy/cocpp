_Pragma("once");

#include "co_define.h"
#include <functional>
#include <list>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_spinlock;

template <typename T>
void wakeup_all_ctx(const std::list<T> ctx_list, std::function<co_ctx*(const T&)> convert)
{
    for (auto& t : ctx_list)
    {
        auto ctx = convert(t);
        ctx->remove_wait_flag();
        ctx->env()->wake_up();
    }
}

template <typename T>
void wakeup_one_ctx(const std::list<T> ctx_list, std::function<co_ctx*(const T&)> convert)
{
    if (ctx_list.empty())
    {
        return;
    }

    auto ctx = convert(ctx_list.front());
    ctx->remove_wait_flag();
    ctx->env()->wake_up();
}

template <typename T>
void add_to_wait_list(std::list<T>& wait_list, T ctx, co_spinlock& mu)
{
    std::lock_guard<co_spinlock> lock(mu);
    if (std::find(wait_list.begin(), wait_list.end(), ctx) == wait_list.end())
    {
        wait_list.push_back(ctx);
    }
}

CO_NAMESPACE_END
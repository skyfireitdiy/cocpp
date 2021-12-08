_Pragma("once");

#include "co_ctx.h"
#include "co_define.h"

#include <deque>
#include <functional>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_spinlock;

void lock_yield__(co_ctx* ctx, co_spinlock& lk, std::function<bool()> checker);

void ctx_enter_wait_state__(co_ctx* ctx, int rc_type, void* rc, std::deque<co_ctx*>& wait_list);

template <typename ContextType>
void ctx_enter_wait_state__(co_ctx* ctx, int rc_type, void* rc, std::deque<ContextType>& wait_list, const ContextType& data)
{
    ctx->enter_wait_rc_state(rc_type, rc);
    wait_list.push_back(data);
}

void wake_front__(std::deque<co_ctx*>& wait_list);

template <typename ContextType>
void wake_front__(std::deque<ContextType>& wait_list, std::function<void(ContextType&)> wake_method)
{
    if (wait_list.empty())
    {
        return;
    }

    auto obj = wait_list.front();
    wait_list.pop_front();
    wake_method(obj);
}

CO_NAMESPACE_END
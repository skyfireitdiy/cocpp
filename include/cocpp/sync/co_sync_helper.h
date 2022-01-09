_Pragma("once");

#include "cocpp/core/co_ctx.h"
#include "cocpp/core/co_define.h"

#include <deque>
#include <functional>

CO_NAMESPACE_BEGIN

class co_ctx;
class co_spinlock;

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
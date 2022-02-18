_Pragma("once");
#include "cocpp/core/co_define.h"
#include "cocpp/utils/co_noncopyable.h"
#include <atomic>
#include <functional>

CO_NAMESPACE_BEGIN

class co_once_flag : private co_noncopyable
{
public:
    std::atomic<bool> flag__ { false }; 
};

template <class Callable, class... Args>
void co_call_once(co_once_flag& flag, Callable&& f, Args&&... args) 
{
    if (!flag.flag__.load(std::memory_order_acquire))
    {
        flag.flag__.store(true, std::memory_order_release);
        std::invoke(std::forward<Callable>(f), std::forward<Args>(args)...);
    }
}

CO_NAMESPACE_END
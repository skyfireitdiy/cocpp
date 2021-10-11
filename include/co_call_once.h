#pragma once
#include "co_define.h"
#include "co_nocopy.h"
#include <atomic>
#include <utility>

CO_NAMESPACE_BEGIN

class co_once_flag : public co_nocopy
{
public:
    std::atomic<bool> flag__ { false };
};

template <class Callable, class... Args>
void co_call_once(co_once_flag& flag, Callable&& f, Args&&... args)
{
    bool v = false;
    if (flag.flag__.compare_exchange_strong(v, true))
    {
        std::forward<Callable>(f)(std::forward<Args>(args)...);
    }
}

CO_NAMESPACE_END
_Pragma("once");

#include <chrono>

#include "cocpp/core/co_define.h"
#include "cocpp/interface/co_this_co.h"

CO_NAMESPACE_BEGIN

template <class Lock>
class co_timed_addition : public Lock
{
public:
    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration); // 尝试加锁
    template <class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& timeout_time); // 尝试加锁
};

// 模板实现

template <class Lock>
template <class Rep, class Period>
bool co_timed_addition<Lock>::try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration)
{
    return try_lock_until(std::chrono::high_resolution_clock::now() + timeout_duration);
}

template <class Lock>
template <class Clock, class Duration>
bool co_timed_addition<Lock>::try_lock_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
{
    do
    {
        if (Lock::try_lock())
        {
            return true;
        }
        co::current_env()->schedule_switch();
    } while (std::chrono::high_resolution_clock::now() < timeout_time);
    return false;
}

CO_NAMESPACE_END
_Pragma("once");

#include <chrono>

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"

CO_NAMESPACE_BEGIN

template <class Lock>
class co_timed_addition : public Lock
{
public:
    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period> &timeout_duration);
    template <class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration> &timeout_time);
};

template <class Lock>
template <class Rep, class Period>
bool co_timed_addition<Lock>::try_lock_for(const std::chrono::duration<Rep, Period> &timeout_duration)
{
    return try_lock_until(std::chrono::steady_clock::now() + timeout_duration);
}

template <class Lock>
template <class Clock, class Duration>
bool co_timed_addition<Lock>::try_lock_until(const std::chrono::time_point<Clock, Duration> &timeout_time)
{
    CoPreemptGuard();
    do
    {
        if (Lock::try_lock())
        {
            return true;
        }
        CoEnablePreempt();
        CoYield();
        CoDisablePreempt();
    } while (std::chrono::steady_clock::now() < timeout_time);
    return false;
}

CO_NAMESPACE_END
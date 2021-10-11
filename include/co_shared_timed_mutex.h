#pragma once

#include <chrono>

#include "co.h"
#include "co_define.h"
#include "co_timed_addition.h"

CO_NAMESPACE_BEGIN

class co_shared_timed_mutex : public co_timed_addition<co_shared_mutex>
{
public:
    template <class Rep, class Period>
    bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout_duration);
    template <class Clock, class Duration>
    bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& timeout_time);
};

template <class Rep, class Period>
bool co_shared_timed_mutex::try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout_duration)
{
    auto start = std::chrono::steady_clock::now();
    do
    {
        if (try_lock_shared())
        {
            return true;
        }
        this_co::yield();
    } while (std::chrono::steady_clock::now() - start < timeout_duration);
    return false;
}

template <class Clock, class Duration>
bool co_shared_timed_mutex::try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
{
    return try_lock_shared_for(timeout_time - std::chrono::steady_clock::now());
}

CO_NAMESPACE_END
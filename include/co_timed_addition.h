#pragma once

#include <chrono>

#include "co.h"

template <class Lock>
class co_timed_addition : public Lock
{
public:
    template <class Rep, class Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration);
    template <class Clock, class Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& timeout_time);
};

template <class Lock>
template <class Rep, class Period>
bool co_timed_addition<Lock>::try_lock_for(const std::chrono::duration<Rep, Period>& timeout_duration)
{
    auto start = std::chrono::steady_clock::now();
    do
    {
        if (Lock::try_lock())
        {
            return true;
        }
        this_co::yield();
    } while (std::chrono::steady_clock::now() - start < timeout_duration);
    return false;
}

template <class Lock>
template <class Clock, class Duration>
bool co_timed_addition<Lock>::try_lock_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
{
    return try_lock_for(timeout_time - std::chrono::steady_clock::now());
}

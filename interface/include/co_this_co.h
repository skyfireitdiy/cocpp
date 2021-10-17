#pragma once

#include "co_defer.h"
#include "co_define.h"
#include "co_type.h"

CO_NAMESPACE_BEGIN

namespace this_co
{
co_id       id();    // 协程id
std::string name();  // 协程名称
void        yield(); // 主动让出cpu
template <class Rep, class Period>
void sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration); // 协程睡眠
template <class Clock, class Duration>
void sleep_until(const std::chrono::time_point<Clock, Duration>& abs_time);
}

// 模板实现

template <class Rep, class Period>
void this_co::sleep_for(const std::chrono::duration<Rep, Period>& sleep_duration) // 协程睡眠
{
    return sleep_until(std::chrono::high_resolution_clock::now() + sleep_duration);
}

template <class Clock, class Duration>
void this_co::sleep_until(const std::chrono::time_point<Clock, Duration>& abs_time)
{
    do
    {
        this_co::yield();
    } while (std::chrono::high_resolution_clock::now() < abs_time);
}

CO_NAMESPACE_END
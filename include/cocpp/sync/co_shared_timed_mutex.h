_Pragma("once");

#include <chrono>

#include "cocpp/core/co_define.h"
#include "cocpp/interface/co.h"
#include "cocpp/sync/co_shared_mutex.h"
#include "cocpp/sync/co_timed_addition.h"

CO_NAMESPACE_BEGIN

class co_shared_timed_mutex : public co_timed_addition<co_shared_mutex>
{
public:
    template <class Rep, class Period>
    bool try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout_duration);     // 尝试加共享锁
    template <class Clock, class Duration>                                                    //
    bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& timeout_time); // 尝试加共享锁
};

// 模板实现

template <class Rep, class Period>
bool co_shared_timed_mutex::try_lock_shared_for(const std::chrono::duration<Rep, Period>& timeout_duration)
{
    return try_lock_shared_until(std::chrono::high_resolution_clock::now() + timeout_duration);
}

template <class Clock, class Duration>
bool co_shared_timed_mutex::try_lock_shared_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
{
    do
    {
        if (try_lock_shared())
        {
            return true;
        }
        co::current_env()->schedule_switch(true);
    } while (std::chrono::high_resolution_clock::now() < timeout_time);
    return false;
}

CO_NAMESPACE_END
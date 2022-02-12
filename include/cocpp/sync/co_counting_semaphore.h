_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/core/co_env.h"
#include "cocpp/core/co_manager.h"
#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/utils/co_noncopyable.h"
#include <cassert>
#include <chrono>

CO_NAMESPACE_BEGIN

template <std::ptrdiff_t LeastMaxValue>
class co_counting_semaphore final : private co_noncopyable
{
private:
    co_mutex              mu__;            // 互斥锁
    co_condition_variable cv_empty__;      // 空闲条件变量
    co_condition_variable cv_full__;       // 满条件变量
    std::ptrdiff_t        desired__;       // 期望值
    void                  release_one__(); // 释放一个信号量
public:
    void acquire();                          // 获取一个信号量
    void release(std::ptrdiff_t update = 1); // 释放一个信号量
    bool try_acquire() noexcept;             // 尝试获取一个信号量
    template <class Rep, class Period>
    bool try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time); // 尝试获取一个信号量
    template <class Clock, class Duration>
    bool try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time); // 尝试获取一个信号量
    constexpr explicit co_counting_semaphore(std::ptrdiff_t desired);                 // 构造函数
    static constexpr std::ptrdiff_t max() noexcept;                                   // 最大值
};

// 模板实现

template <std::ptrdiff_t LeastMaxValue>
void co_counting_semaphore<LeastMaxValue>::acquire()
{
    std::unique_lock lock(mu__);
    assert(desired__ >= 0 && desired__ <= LeastMaxValue);
    while (desired__ == 0)
    {
        cv_empty__.wait(lock);
    }
    --desired__;
    cv_full__.notify_one();
}

template <std::ptrdiff_t LeastMaxValue>
void co_counting_semaphore<LeastMaxValue>::release_one__()
{
    std::unique_lock lock(mu__);
    assert(desired__ >= 0 && desired__ <= LeastMaxValue);
    while (desired__ == LeastMaxValue)
    {
        cv_full__.wait(lock);
    }
    ++desired__;
    cv_empty__.notify_one();
}

template <std::ptrdiff_t LeastMaxValue>
void co_counting_semaphore<LeastMaxValue>::release(std::ptrdiff_t update)
{
    for (std::ptrdiff_t i = 0; i < update; ++i)
    {
        release_one__();
    }
}

template <std::ptrdiff_t LeastMaxValue>
bool co_counting_semaphore<LeastMaxValue>::try_acquire() noexcept
{
    std::unique_lock lock(mu__);
    assert(desired__ >= 0 && desired__ <= LeastMaxValue);
    if (desired__ == 0)
    {
        return false;
    }
    --desired__;
    cv_full__.notify_one();
    return true;
}

template <std::ptrdiff_t LeastMaxValue>
template <class Rep, class Period>
bool co_counting_semaphore<LeastMaxValue>::try_acquire_for(const std::chrono::duration<Rep, Period>& rel_time)
{
    return try_acquire_until(std::chrono::steady_clock::now() + rel_time);
}

template <std::ptrdiff_t LeastMaxValue>
template <class Clock, class Duration>
bool co_counting_semaphore<LeastMaxValue>::try_acquire_until(const std::chrono::time_point<Clock, Duration>& abs_time)
{
    do
    {
        if (try_acquire())
        {
            return true;
        }
        co_manager::instance()->current_env()->schedule_switch();
    } while (std::chrono::steady_clock::now() < abs_time);
    return false;
}

template <std::ptrdiff_t LeastMaxValue>
constexpr std::ptrdiff_t co_counting_semaphore<LeastMaxValue>::max() noexcept
{
    return LeastMaxValue;
}

template <std::ptrdiff_t LeastMaxValue>
constexpr co_counting_semaphore<LeastMaxValue>::co_counting_semaphore(std::ptrdiff_t desired)
    : desired__(desired)
{
    assert(desired >= 0 && desired <= LeastMaxValue);
}

CO_NAMESPACE_END
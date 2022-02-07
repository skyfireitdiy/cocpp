_Pragma("once");

#include "cocpp/core/co_define.h"
#include "cocpp/sync/co_condition_variable.h"
#include "cocpp/sync/co_mutex.h"
#include "cocpp/utils/co_noncopyable.h"

#include <exception>
#include <future>

CO_NAMESPACE_BEGIN

template <typename T>
class co_promise;

template <typename T>
class co_future final : private co_noncopyable
{
private:
    bool               is_ready__;
    bool               is_valid__;
    T                  value__;
    std::exception_ptr exception__;

    co_mutex              mutex__;
    co_condition_variable get_cond__;

    void set_value__(const T& value);
    void set_exception__(std::exception_ptr p);

public:
    T    get() const;
    bool valid() const noexcept;
    void wait() const;
    template <class Rep, class Period>
    std::future_status wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const;
    template <class Clock, class Duration>
    std::future_status wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time) const;
};

// 模板实现

template <typename T>
bool co_future<T>::valid() const noexcept
{
    std::scoped_lock lock(mutex__);
    return is_valid__;
}

template <typename T>
T co_future<T>::get() const
{
    std::scoped_lock lock(mutex__);
    if (!valid())
    {
        throw std::future_error(std::future_errc::no_state);
    }
    wait();
    return value__;
}

template <typename T>
void co_future<T>::wait() const
{
    std::scoped_lock lock(mutex__);
    if (!valid())
    {
        throw std::future_error(std::future_errc::no_state);
    }
    if (!is_ready__)
    {
        get_cond__.wait(lock, [this] { return is_ready__; });
    }
    if (exception__)
    {
        std::rethrow_exception(exception__);
    }
}

template <typename T>
template <class Rep, class Period>
std::future_status co_future<T>::wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const
{
    return wait_until(std::chrono::steady_clock::now() + timeout_duration);
}

template <typename T>
template <class Clock, class Duration>
std::future_status co_future<T>::wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time) const
{
    std::scoped_lock lock(mutex__);
    if (!valid())
    {
        throw std::future_error(std::future_errc::no_state);
    }
    if (!is_ready__)
    {
        if (!get_cond__.wait_until(lock, timeout_time, [this] { return is_ready__; }))
        {
            return std::future_status::timeout;
        }
    }
    if (exception__)
    {
        std::rethrow_exception(exception__);
    }
    return std::future_status::ready;
}

template <typename T>
void co_future<T>::set_value__(const T& value)
{
    std::scoped_lock lock(mutex__);
    if (!valid())
    {
        throw std::future_error(std::future_errc::no_state);
    }
    if (is_ready__)
    {
        throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    value__    = value;
    is_ready__ = true;
    get_cond__.notify_all();
}

template <typename T>
void co_future<T>::set_exception__(std::exception_ptr p)
{
    std::scoped_lock lock(mutex__);
    if (!valid())
    {
        throw std::future_error(std::future_errc::no_state);
    }
    if (is_ready__)
    {
        throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    exception__ = p;
    is_ready__  = true;
    get_cond__.notify_one();
}

CO_NAMESPACE_END
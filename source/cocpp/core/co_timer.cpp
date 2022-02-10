#include "cocpp/core/co_timer.h"

CO_NAMESPACE_BEGIN

co_timer::co_timer(const std::function<void()>& func, co_timeout_type type, unsigned long long interval_ms)
    : handle__(static_cast<co_timer_handle>(reinterpret_cast<unsigned long>(this)))
    , callback__(func)
    , timeout_type__(type)
    , timer_type__(co_timer_type::absolute)
    , interval__(interval_ms)
    , timeout_time__(std::chrono::steady_clock::now() + std::chrono::milliseconds(interval_ms))
{
}

co_timer::co_timer(const std::function<void()>& func, std::chrono::steady_clock::time_point timeout_time)
    : handle__(static_cast<co_timer_handle>(reinterpret_cast<unsigned long>(this)))
    , callback__(func)
    , timeout_type__(co_timeout_type::once)
    , timer_type__(co_timer_type::relative)
    , interval__(0)
    , timeout_time__(timeout_time)
{
}

std::shared_ptr<co_timer> co_timer::create(const std::function<void()>& func, co_timeout_type type, unsigned long long interval_ms)
{
    return std::make_shared<co_timer>(func, type, interval_ms);
}

bool operator==(const std::shared_ptr<co_timer>& lhs, const std::shared_ptr<co_timer>& rhs)
{
    return lhs->get_handle() == rhs->get_handle();
}

bool operator<(const std::shared_ptr<co_timer>& lhs, const std::shared_ptr<co_timer>& rhs)
{
    return lhs->get_handle() < rhs->get_handle();
}

CO_NAMESPACE_END
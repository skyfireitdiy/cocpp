#include "cocpp/core/co_timer.h"
#include "cocpp/core/co_manager.h"

CO_NAMESPACE_BEGIN

co_timer::co_timer(const std::function<void()>& func, co_timeout_type type, unsigned long long interval_ms)
    : handle__(static_cast<co_timer_handle>(reinterpret_cast<unsigned long>(this)))
    , callback__(func)
    , timeout_type__(type)
    , timer_type__(co_timer_type::absolute)
    , interval__(interval_ms)
    , status__(co_timer_status::stopped)
{
}

co_timer::co_timer(const std::function<void()>& func, std::chrono::steady_clock::time_point timeout_time)
    : handle__(static_cast<co_timer_handle>(reinterpret_cast<unsigned long>(this)))
    , callback__(func)
    , timeout_type__(co_timeout_type::once)
    , timer_type__(co_timer_type::relative)
    , interval__(0)
    , timeout_time__(timeout_time)
    , status__(co_timer_status::stopped)
{
}

std::shared_ptr<co_timer> co_timer::create(const std::function<void()>& func, co_timeout_type type, unsigned long long interval_ms)
{
    return std::make_shared<co_timer>(func, type, interval_ms);
}

co_timer_handle co_timer::get_handle() const
{
    return handle__;
}

bool co_timer::is_running() const
{
    std::scoped_lock lock(mutex__);
    return status__ == co_timer_status::running;
}

void co_timer::start()
{
    std::scoped_lock lock(mutex__);
    if (status__ == co_timer_status::running)
    {
        return;
    }
    update_timeout_time__();
    status__ = co_timer_status::running;
    insert_to_timer_queue__();
}

void co_timer::stop()
{
    std::scoped_lock lock(mutex__);
    if (status__ == co_timer_status::stopped)
    {
        return;
    }

    status__ = co_timer_status::stopped;
    remove_from_timer_queue__();
}

void co_timer::reset()
{
    stop();
    start();
}

void co_timer::update_timeout_time__()
{
    if (timer_type__ == co_timer_type::relative)
    {
        timeout_time__ = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval__);
    }
}

void co_timer::insert_to_timer_queue__()
{
    co_manager::instance()->insert_timer_to_queue__(shared_from_this());
}

void co_timer::remove_from_timer_queue__()
{
    co_manager::instance()->remove_timer_from_queue__(shared_from_this());
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
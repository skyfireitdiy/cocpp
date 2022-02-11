#include "cocpp/core/co_timer.h"
#include "cocpp/core/co_manager.h"

CO_NAMESPACE_BEGIN

co_timer::co_timer(const std::function<void()>& func, co_expire_type type, unsigned long long interval_ms)
    : handle__(static_cast<co_timer_handle>(reinterpret_cast<unsigned long>(this)))
    , callback__(func)
    , expire_type__(type)
    , timer_type__(co_timer_type::absolute)
    , interval__(interval_ms)
    , status__(co_timer_status::stopped)
{
}

co_timer::co_timer(const std::function<void()>& func, std::chrono::steady_clock::time_point expire_time)
    : handle__(static_cast<co_timer_handle>(reinterpret_cast<unsigned long>(this)))
    , callback__(func)
    , expire_type__(co_expire_type::once)
    , timer_type__(co_timer_type::relative)
    , interval__(0)
    , expire_time__(expire_time)
    , status__(co_timer_status::stopped)
{
}

std::shared_ptr<co_timer> co_timer::create(const std::function<void()>& func, co_expire_type type, unsigned long long interval_ms)
{
    return std::shared_ptr<co_timer>(new co_timer(func, type, interval_ms));
}

std::shared_ptr<co_timer> co_timer::create(const std::function<void()>& func, std::chrono::steady_clock::time_point expire_time)
{
    return std::shared_ptr<co_timer>(new co_timer(func, expire_time));
}

co_expire_type co_timer::expire_type() const
{
    return expire_type__;
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
        expire_time__ = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval__);
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

std::chrono::steady_clock::time_point co_timer::expire_time() const
{
    std::scoped_lock lock(mutex__);
    return expire_time__;
}

co_timer_type co_timer::timer_type() const
{
    return timer_type__;
}

bool co_timer::is_expired() const
{
    std::scoped_lock lock(mutex__);
    return std::chrono::steady_clock::now() >= expire_time__;
}

void co_timer::run() const
{
    callback__();
}

bool operator==(const std::shared_ptr<co_timer>& lhs, const std::shared_ptr<co_timer>& rhs)
{
    return lhs->get_handle() == rhs->get_handle();
}

bool operator<(const std::shared_ptr<co_timer>& lhs, const std::shared_ptr<co_timer>& rhs)
{
    return lhs->expire_time() < rhs->expire_time();
}

CO_NAMESPACE_END
#include "cocpp/sync/co_barrier.h"

CO_NAMESPACE_BEGIN

co_barrier::co_barrier(std::ptrdiff_t expected)
    : expected__(expected)
    , count__(expected)
{
}

co_arrival_token co_barrier::arrive(std::ptrdiff_t n)
{
    co_arrival_token arrival;
    arrival.count__ = n;
    arrival.mutex__.lock();
    if (count__.fetch_add(n, std::memory_order_relaxed) == expected__)
    {
        count__.store(0, std::memory_order_release);
        cond__.notify_all();
    }
    else
    {
        while (count__.load(std::memory_order_acquire) != 0)
        {
            arrival.condition__.wait(arrival.mutex__);
        }
    }
    return arrival;
}

void co_barrier::wait(co_arrival_token&& arrival) const
{
    if (arrival.count__ == 0)
    {
        return;
    }
    arrival.mutex__.lock();
    if (count__.load(std::memory_order_acquire) == 0)
    {
        arrival.mutex__.unlock();
        return;
    }
    while (count__.load(std::memory_order_acquire) != 0)
    {
        arrival.condition__.wait(arrival.mutex__);
    }
    arrival.mutex__.unlock();
}

void co_barrier::arrive_and_wait()
{
    auto arrival = arrive();
    wait(std::move(arrival));
}

void co_barrier::arrive_and_drop()
{
    auto arrival = arrive();
    arrival.mutex__.unlock();
}

CO_NAMESPACE_END
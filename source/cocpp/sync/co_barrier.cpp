#include "cocpp/sync/co_barrier.h"

CO_NAMESPACE_BEGIN

co_barrier::co_barrier(std::ptrdiff_t expected)
    : expected__(expected)
    , count__(expected)
{
}

co_arrival_token co_barrier::arrive(std::ptrdiff_t n)
{
    std::scoped_lock lock(mutex__);
    if (count__ - n < 0)
    {
        throw std::logic_error("co_barrier::arrive: count < 0");
    }
    count__ -= n;
    return co_arrival_token {
        generation__,
        this
    };
}

void co_barrier::wait(co_arrival_token&& arrival)
{
    std::unique_lock lock(mutex__);
    if (arrival.generation__ != generation__ || arrival.barrier__ != this)
    {
        throw std::logic_error("co_barrier::wait: invalid token");
    }
    if (count__ == 0)
    {
        generation__++;
        count__ = expected__;
        cond__.notify_all();
    }
    else
    {
        cond__.wait(lock, [&] { return count__ == 0; });
    }
}

void co_barrier::arrive_and_wait()
{
    wait(arrive());
}

void co_barrier::arrive_and_drop()
{
    arrive();
}

CO_NAMESPACE_END
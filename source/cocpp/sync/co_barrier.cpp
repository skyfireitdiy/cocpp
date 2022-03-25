#include "cocpp/sync/co_barrier.h"

CO_NAMESPACE_BEGIN

co_barrier::co_barrier(std::ptrdiff_t expected)
    : expected__(expected)
    , count__(expected)
    , max__(expected)
{
}

co_arrival_token co_barrier::arrive(std::ptrdiff_t n)
{
    std::scoped_lock lock(mutex__);
    if (expected__ - n < 0)
    {
        throw std::logic_error("co_barrier::arrive: count < 0");
    }
    expected__ -= n;
    return co_arrival_token {
        .generation__ = generation__,
        .barrier__    = this,
        .n__          = n
    };
}

void co_barrier::wait(co_arrival_token&& arrival)
{
    std::unique_lock lock(mutex__);
    if (arrival.generation__ != generation__ || arrival.barrier__ != this)
    {
        throw std::logic_error("co_barrier::wait: invalid token");
    }
    if (count__ - arrival.n__ < 0)
    {
        throw std::logic_error("co_barrier::wait: count < 0");
    }
    count__ -= arrival.n__;
    if (count__ == 0)
    {
        generation__++;
        count__    = max__;
        expected__ = max__;
        cond__.notify_all();
    }
    else
    {
        cond__.wait(lock);
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
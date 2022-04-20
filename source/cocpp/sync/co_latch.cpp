#include "cocpp/sync/co_latch.h"

using namespace std;

CO_NAMESPACE_BEGIN

co_latch::co_latch(ptrdiff_t expected)
    : expect__(expected)
    , mutex__()
    , cond__()
{
}

void co_latch::count_down(ptrdiff_t n)
{
    scoped_lock lock(mutex__);
    if (expect__ < n)
    {
        throw logic_error("co_latch::count_down: expected < n");
    }
    expect__ -= n;
    if (expect__ == 0)
    {
        cond__.notify_all();
    }
}

bool co_latch::try_wait() const noexcept
{
    scoped_lock lock(mutex__);
    return expect__ == 0;
}

void co_latch::wait() const
{
    unique_lock lock(mutex__);
    while (expect__ > 0)
    {
        cond__.wait(lock);
    }
}

void co_latch::arrive_and_wait(ptrdiff_t n)
{
    count_down(n);
    wait();
}

CO_NAMESPACE_END